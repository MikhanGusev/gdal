/******************************************************************************
 *
 * Project:  GDAL NextGIS Web (NGW) vector-raster driver
 * Purpose:  ...
 * Author:   Mikhail Gusev, gusevmihs@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2016, 2017, NextGIS
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "ogr_ngw.h"


OGRNGWLayer::OGRNGWLayer (OGRNGWDataSource *poDSIn, int nNgwIdIn):
    poDS(poDSIn),
    poFeatureDefn(NULL),
    bIsMetaCached(false),
    bAreFeaturesCached(false),
    nNgwId(nNgwIdIn),
    nCurFeature(-1)
{
}

OGRNGWLayer::~OGRNGWLayer ()
{
    // Destroy all cached features of this layer.
    for (int i=0; i<vCachedFeatures.size(); i++)
        delete vCachedFeatures[i];

    if (poFeatureDefn != NULL)
        poFeatureDefn->Release();
}


// Resets current feature's index and caches features storing them all in memory (at
// the first time).
void OGRNGWLayer::ResetReading ()
{
    if (!bIsMetaCached)
    {
        CPLDebug("NGW", "Unable to get features. The layer is void "
                        "(need to get NGW resource metadata first)");
        return;
    }

    this->cacheFeatures(); // can be a long operation at first

    // FUTURE: Here can be an update of cached features.

    nCurFeature = 0;
}


// Returns next feature from the array of cached features.
OGRFeature *OGRNGWLayer::GetNextFeature ()
{
    if (nCurFeature == -1)
        return NULL;

    if (nCurFeature == vCachedFeatures.size()) // end of array of features
        return NULL;

    OGRFeature *poFeature = vCachedFeatures[nCurFeature];
    nCurFeature++;
    return poFeature->Clone(); // return copy so can be possible to destroy outside in a common way
}


OGRFeatureDefn *OGRNGWLayer::GetLayerDefn ()
{
    return poFeatureDefn;
}


int OGRNGWLayer::TestCapability (const char *)
{
    return FALSE;
}


// Load all layer's metadata into memory.
void OGRNGWLayer::cacheMeta ()
{
    if (bIsMetaCached)
        return;

    if (poDS == NULL)
    {
        CPLDebug("NGW", "Unable to cache resource's metadata: void dataset pointer");
        return;
    }

    if (bAreFeaturesCached)
    {
        CPLDebug("NGW", "There are already some features cached. Unable to update layers's metadata");
        return;
    }

    // Perform HTTP request.
    json_object *jsonMeta = poDS->poNgwWorker->getResourceMeta(nNgwId);
    if (jsonMeta == NULL)
    {
        CPLDebug("NGW", "Unable to cache layers's metadata: incorrect JSON received");
        return;
    }

    // Create layer defintion based on its name.
    // NOTE: for NGW driver the layer's name is NGW resource id!
    const char *pszLayerName = CPLSPrintf("%d",nNgwId); // convert int to string
    poFeatureDefn = new OGRFeatureDefn(pszLayerName);
    this->SetDescription(poFeatureDefn->GetName());
    poFeatureDefn->Reference();
    // QUESTION: should we free pszLayerName here?

    // Parse geometry info.
    json_object *jsonGeom = poDS->poNgwWorker->takeResourceGeomType(jsonMeta);
    if (jsonGeom == NULL)
    {
//        json_object_put(jsonMeta);
        CPLDebug("NGW", "Incorrect JSON with resource's metadata: no geometry info");
        return;
    }

    // Parse SRS info.
    json_object *jsonSrs = poDS->poNgwWorker->takeResourceSrs(jsonMeta);
    if (jsonSrs == NULL)
    {
//        json_object_put(jsonMeta);
        CPLDebug("NGW", "Incorrect JSON with resource's metadata: no SRS info");
        return;
    }

    // Parse fields info.
    json_object *jsonFields = poDS->poNgwWorker->takeResourceFields(jsonMeta);
    if (jsonFields == NULL)
    {
//        json_object_put(jsonMeta);
        CPLDebug("NGW", "Incorrect JSON with resource's metadata: no SRS info");
        return;
    }

    const char *pszGeomtype = json_object_get_string(jsonGeom);
    int nSrs = json_object_get_int(jsonSrs);

    // QUESTION: ok to name geometry field as "geometry"? It seems that it is not important.
    OGRGeomFieldDefn *poGeomDefn
            = new OGRGeomFieldDefn("geometry", NGWWorker::geomTypeOgr(pszGeomtype));
    OGRSpatialReference *poSrs = new OGRSpatialReference();
    OGRErr errSrs = poSrs->importFromEPSG(nSrs);
    poGeomDefn->SetSpatialRef(poSrs);
    poFeatureDefn->DeleteGeomFieldDefn(0); // firstly delete default created geom field!
    poFeatureDefn->AddGeomFieldDefn(poGeomDefn,FALSE);

    if (poFeatureDefn->GetGeomType() == wkbUnknown)
        CPLDebug("NGW", "Warning: unknown geometry of resource %d", nNgwId);
    if (errSrs != OGRERR_NONE)
        CPLDebug("NGW", "Warning: unknown SRS type of resource %d", nNgwId);

    int nSkipped = 0;
    int nSize = json_object_array_length(jsonFields);
    for (int i=0; i<nSize; i++)
    {
        json_object *jsonField = json_object_array_get_idx(jsonFields, i);
        json_object *jsonKeyname = poDS->poNgwWorker->takeFieldName(jsonField);
        json_object *jsonDatatype = poDS->poNgwWorker->takeFieldType(jsonField);

        if (jsonKeyname == NULL || jsonDatatype == NULL)
        {
            nSkipped++;
            continue;
        }

        const char *pszKeyname = json_object_get_string(jsonKeyname);
        const char *pszDatatype = json_object_get_string(jsonDatatype);

        OGRFieldDefn oFieldDefn(pszKeyname, NGWWorker::dataTypeOgr(pszDatatype));
        poFeatureDefn->AddFieldDefn(&oFieldDefn);
    }

    if (nSkipped > 0)
    {
        CPLDebug("NGW", "Skipped %d field(s) for the resource %s because of errors during JSON parsing",
                 nSkipped, pszLayerName);
    }

    // Parse layer's display name.
    json_object *jsonDispname = poDS->poNgwWorker->takeResourceDispName(jsonMeta);
    if (jsonDispname == NULL)
    {
        CPLDebug("NGW", "Warning: unable to read display name for resource %d", nNgwId);
    }
    else
    {
        const char *pszDispname = json_object_get_string(jsonDispname);
        this->SetMetadataItem("DISPLAY_NAME", pszDispname);
    }

    CPLDebug("NGW", "Cached metadata of resource %d", nNgwId);

//    json_object_put(jsonMeta);

    bIsMetaCached = true;
}


// Load all layer's features into memory.
void OGRNGWLayer::cacheFeatures ()
{
    if (poFeatureDefn == NULL)
        return;

    if (bAreFeaturesCached)
        return;

    if (poDS == NULL)
    {
        CPLDebug("NGW", "Unable to cache features for the layer: void dataset pointer");
        return;
    }

    // Perform HTTP request.
    json_object *jsonFeats = poDS->poNgwWorker->getResourceFeatures(nNgwId);
    if (jsonFeats == NULL)
    {
        CPLDebug("NGW", "Unable to cache features for the layer: incorrect JSON received");
        return;
    }

    // Look through the array of features of the resource.
    int nSkipped = 0;
    int nSize = json_object_array_length(jsonFeats);
    for (int i = 0; i < nSize; i++)
    {
        json_object *jsonFeat = json_object_array_get_idx(jsonFeats, i);

        OGRFeature *poFeature = new OGRFeature(poFeatureDefn);

        // Parse and set feature's fid.
        json_object *jsonId = poDS->poNgwWorker->takeFeatureId(jsonFeat);
        if (jsonId == NULL)
        {
            nSkipped++;
            delete poFeature;
            continue;
        }
        GIntBig nId = json_object_get_int64(jsonId);
        poFeature->SetFID(nId);

        // Parse feature's geometry.
        json_object *jsonGeom = poDS->poNgwWorker->takeFeatureGeom(jsonFeat);
        if (jsonGeom == NULL)
        {
            nSkipped++;
            delete poFeature;
            continue;
        }
        const char *pszGeom = json_object_get_string(jsonGeom);

        // Get geometry type from the layer description.
        OGRwkbGeometryType eGeom = poFeatureDefn->GetGeomType();
        OGRGeometry *poGeom;
        switch (eGeom)
        {
            case wkbPoint: poGeom = new OGRPoint(); break;
            case wkbLineString: poGeom = new OGRLineString(); break;
            case wkbPolygon: poGeom = new OGRPolygon(); break;
            case wkbMultiPoint: poGeom = new OGRMultiPoint(); break;
            case wkbMultiLineString: poGeom = new OGRMultiLineString(); break;
            case wkbMultiPolygon: poGeom = new OGRMultiPolygon(); break;
            default: poGeom = new OGRPoint(); break; // will return error during the import from WKT
        }

        // Import geometry from the WKT JSON string.
        char *pszGeomDup = CPLStrdup(pszGeom); // so not to corrupt json-c inner string
        OGRErr errGeom = poGeom->importFromWkt(&pszGeomDup); // will modify input string!
        if (errGeom != OGRERR_NONE)
        {
            nSkipped++;
            delete poGeom;
            delete poFeature;
            continue;
        }
        poFeature->SetGeometryDirectly(poGeom);

        // Parse feature's field values.
        json_object *jsonFields = poDS->poNgwWorker->takeFeatureAttrs(jsonFeat);
        if (jsonFields == NULL)
        {
            nSkipped++;
            delete poFeature;
            continue;
        }

        // Iterate over all fields and write their values to the OGRFeature.
        // NOTE: if we skip a field it will remain NULL for the feature. Usually such things must
        // not happen because the fields' structure was correctly defined during the cacheMeta()
        // method - we do it in order to catch some unpredictable errors.
        json_object_iterator iterFields = json_object_iter_begin(jsonFields);
        json_object_iterator iterFieldsEnd = json_object_iter_end(jsonFields);
        while (!json_object_iter_equal(&iterFields, &iterFieldsEnd))
        {
            // Firstly we need to know the GDAL type of the field which is stored in OGRFeatureDefn.
            const char *pszFildname = json_object_iter_peek_name(&iterFields);
            int nFieldInd = poFeatureDefn->GetFieldIndex(pszFildname);
            OGRFieldDefn *poFieldDefn = poFeatureDefn->GetFieldDefn(nFieldInd);
            if (poFieldDefn == NULL)
            {
                json_object_iter_next(&iterFields);
                continue;
            }
            OGRFieldType eFieldtype = poFieldDefn->GetType();

            // Get the field of some JSON type.
            json_object *jsonField = json_object_iter_peek_value(&iterFields);
            if (jsonField == NULL)
            {
                json_object_iter_next(&iterFields);
                continue;
            }

            // Check that it is not a JSON-null type. Otherwise it is a normal way to skip field
            // so it will remain NULL (= in unset state) in OGRFeature.
            json_type typeField = json_object_get_type(jsonField);
            if (typeField == json_type_null)
            {
                json_object_iter_next(&iterFields);
                continue;
            }

            // Finally set the field's value of OGRFeature according to its GDAL type.
            if (eFieldtype == OFTString)
            {
                const char *pszFieldvalue = json_object_get_string(jsonField);
                poFeature->SetField(nFieldInd, pszFieldvalue);
            }
            else if (eFieldtype == OFTInteger64)
            {
                GInt64 nFieldvalue = json_object_get_int64(jsonField);
                poFeature->SetField(nFieldInd, nFieldvalue);
            }
            else if (eFieldtype == OFTReal)
            {
                double dFieldvalue = json_object_get_double(jsonField);
                poFeature->SetField(nFieldInd, dFieldvalue);
            }
            else if (eFieldtype == OFTDate)
            {
                NGWDateTime oDateTime = poDS->poNgwWorker->readDateTime(jsonField);
                poFeature->SetField(nFieldInd, oDateTime.nYear, oDateTime.nMonth, oDateTime.nDay);
            }
            else if (eFieldtype == OFTDateTime || eFieldtype == OFTTime)
            {
                NGWDateTime oDateTime = poDS->poNgwWorker->readDateTime(jsonField);
                poFeature->SetField(nFieldInd, oDateTime.nYear, oDateTime.nMonth, oDateTime.nDay,
                                    oDateTime.nHour, oDateTime.nMinute, oDateTime.nSecond);
            }
            // Else - hardly possible but anyway skip to leave NULL value.

            json_object_iter_next(&iterFields);
        }

        // Add newly created feature to the array of cached features.
        vCachedFeatures.push_back(poFeature);
    }

    if (nSkipped > 0)
    {
        CPLDebug("NGW", "Skipped %d features in the layer %s because of errors during parsing of JSON",
                 nSkipped, this->GetLayerDefn()->GetName());
    }

//    json_object_put(jsonFeats);

    bAreFeaturesCached = true;
}



