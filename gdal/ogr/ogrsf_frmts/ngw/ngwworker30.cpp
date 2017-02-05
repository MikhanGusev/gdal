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

#include "ngwapi.h"


NGWWorker30::NGWWorker30 (const char *pszUrl):
    NGWWorker(pszUrl)
{
    m_urlFlatList = "/resource/store/";
    m_urlResource = "/api/resource/%d";
    m_urlFeatures = "/api/resource/%d/feature/";
}

NGWWorker30::~NGWWorker30()
{

}


NGWDateTime NGWWorker30::readDateTime (json_object *jsonObj)
{
    NGWDateTime ret;
    json_object *jsonYear = CPL_json_object_object_get(jsonObj, "year");
    json_object *jsonMonth = CPL_json_object_object_get(jsonObj, "month");
    json_object *jsonDay = CPL_json_object_object_get(jsonObj, "day");
    json_object *jsonHour = CPL_json_object_object_get(jsonObj, "hour");
    json_object *jsonMin = CPL_json_object_object_get(jsonObj, "min");
    json_object *jsonSec = CPL_json_object_object_get(jsonObj, "sec");
    ret.nYear = json_object_get_int(jsonYear);
    ret.nMonth = json_object_get_int(jsonMonth);
    ret.nDay = json_object_get_int(jsonDay);
    ret.nHour = json_object_get_int(jsonHour);
    ret.nMinute = json_object_get_int(jsonMin);
    ret.nSecond = json_object_get_int(jsonSec);
    return ret;
}


json_object *NGWWorker30::getVectorResources ()
{
    std::string urlList = m_urlBase + m_urlFlatList;
    json_object *jsonList = httpGet(urlList.c_str());
    if (!_correctJsonArray(jsonList)) // TODO: check that array contains objects
        return NULL;
    return jsonList;
}

json_object *NGWWorker30::getResourceMeta (int nResourceId)
{
    std::string urlMeta = m_urlBase + CPLSPrintf(m_urlResource.c_str(), nResourceId);
    json_object *jsonMeta = httpGet(urlMeta.c_str());
    if (!_correctJsonObject(jsonMeta))
        return NULL;
    return jsonMeta;
}

json_object *NGWWorker30::getResourceFeatures (int nResourceId)
{
    std::string urlFeatures = m_urlBase + CPLSPrintf(m_urlFeatures.c_str(), nResourceId);
    json_object *jsonFeatures = httpGet(urlFeatures.c_str());
    if (!_correctJsonArray(jsonFeatures)) // TODO: check that array contains objects
        return NULL;
    return jsonFeatures;
}


json_object *NGWWorker30::takeResourceType (json_object *jsonResourceItem)
{
    if (!_correctJsonObject(jsonResourceItem))
        return NULL;
    json_object *jsonType = CPL_json_object_object_get(jsonResourceItem, "cls");
    if (!_correctJsonString(jsonType))
        return NULL;
    return jsonType;
}

json_object *NGWWorker30::takeResourceId (json_object *jsonResourceItem)
{
    if (!_correctJsonObject(jsonResourceItem))
        return NULL;
    json_object *jsonId = CPL_json_object_object_get(jsonResourceItem, "id");
    if (!_correctJsonInt(jsonId))
        return NULL;
    return jsonId;
}

json_object *NGWWorker30::takeResourceGeomType (json_object *jsonResource)
{
    if (!_correctJsonObject(jsonResource))
        return NULL;
    json_object *jsonVecLayer = CPL_json_object_object_get(jsonResource, "vector_layer");
    if (!_correctJsonObject(jsonVecLayer))
        return NULL;
    json_object *jsonGeomType = CPL_json_object_object_get(jsonVecLayer, "geometry_type");
    if (!_correctJsonString(jsonGeomType))
        return NULL;
    return jsonGeomType;

    // TODO: add other ways to get geometry type if this is a postgis_layer or wfsserver_service
    // resource types.
}

json_object *NGWWorker30::takeResourceSrs (json_object *jsonResource)
{
    if (!_correctJsonObject(jsonResource))
        return NULL;
    json_object *jsonVecLayer = CPL_json_object_object_get(jsonResource, "vector_layer");
    if (!_correctJsonObject(jsonVecLayer))
        return NULL;
    json_object *jsonSrs = CPL_json_object_object_get(jsonVecLayer, "srs");
    if (!_correctJsonObject(jsonSrs))
        return NULL;
    json_object *jsonSrsId = CPL_json_object_object_get(jsonSrs, "id");
    if (!_correctJsonInt(jsonSrsId))
        return NULL;
    return jsonSrsId;

    // TODO: add other ways to get geometry type if this is a postgis_layer or wfsserver_service
    // resource types.
}

json_object *NGWWorker30::takeResourceFields (json_object *jsonResource)
{
    if (!_correctJsonObject(jsonResource))
        return NULL;
    json_object *jsonFeatLayer = CPL_json_object_object_get(jsonResource, "feature_layer");
    if (!_correctJsonObject(jsonFeatLayer))
        return NULL;
    json_object *jsonFields = CPL_json_object_object_get(jsonFeatLayer, "fields");
    if (!_correctJsonArray(jsonFields)) // TODO: check that array contains objects
        return NULL;
    return jsonFields;
}

json_object *NGWWorker30::takeResourceDispName (json_object *jsonResource)
{
    if (!_correctJsonObject(jsonResource))
        return NULL;
    json_object *jsonRes = CPL_json_object_object_get(jsonResource, "resource");
    if (!_correctJsonObject(jsonRes))
        return NULL;
    json_object *jsonDispName = CPL_json_object_object_get(jsonRes, "display_name");
    if (!_correctJsonString(jsonDispName))
        return NULL;
    return jsonDispName;
}

json_object *NGWWorker30::takeFieldType (json_object *jsonField)
{
    if (!_correctJsonObject(jsonField))
        return NULL;
    json_object *jsonType = CPL_json_object_object_get(jsonField, "datatype");
    if (!_correctJsonString(jsonType))
        return NULL;
    return jsonType;
}

json_object *NGWWorker30::takeFieldName (json_object *jsonField)
{
    if (!_correctJsonObject(jsonField))
        return NULL;
    json_object *jsonName = CPL_json_object_object_get(jsonField, "keyname");
    if (!_correctJsonString(jsonName))
        return NULL;
    return jsonName;
}

json_object *NGWWorker30::takeFeatureId (json_object *jsonFeature)
{
    if (!_correctJsonObject(jsonFeature))
        return NULL;
    json_object *jsonId = CPL_json_object_object_get(jsonFeature, "id");
    if (!_correctJsonInt(jsonId))
        return NULL;
    return jsonId;
}

json_object *NGWWorker30::takeFeatureGeom (json_object *jsonFeature)
{
    if (!_correctJsonObject(jsonFeature))
        return NULL;
    json_object *jsonGeom = CPL_json_object_object_get(jsonFeature, "geom");
    if (!_correctJsonString(jsonGeom))
        return NULL;
    return jsonGeom;
}

json_object *NGWWorker30::takeFeatureAttrs (json_object *jsonFeature)
{
    if (!_correctJsonObject(jsonFeature))
        return NULL;
    json_object *jsonFields = CPL_json_object_object_get(jsonFeature, "fields");
    if (!_correctJsonObject(jsonFields))
        return NULL;
    return jsonFields;
}



