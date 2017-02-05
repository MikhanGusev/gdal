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


OGRNGWDataSource::OGRNGWDataSource ()
    // QUESTION: why not to call parent constructor here?
{
    poNgwWorker = NULL;
}


OGRNGWDataSource::~OGRNGWDataSource ()
{
    for(size_t i = 0; i < vLayers.size(); i++)
        delete vLayers[i]; // this will also delete all cached features and metadata

    if (poNgwWorker != NULL)
        delete poNgwWorker;
}


int OGRNGWDataSource::Open (const char *pszFilename, char** papszOpenOptionsIn, int bUpdateIn)
{
    if (bUpdateIn)
    {
        CPLError(CE_Failure, CPLE_OpenFailed,
                 "Update access currently is not supported by NGW driver");
        return FALSE;
    }

    // QUESTION: do we need this for checking double opening?
    if (poNgwWorker != NULL)
    {
        CPLError(CE_Failure, CPLE_OpenFailed, "This datasource is already opened");
        return FALSE;
    }

    if (STARTS_WITH_CI(pszFilename, "NEXTGISWEB:"))
        sName = CPLStrdup(pszFilename + strlen("NEXTGISWEB:"));
    else if (STARTS_WITH_CI(pszFilename, "NGW:"))
        sName = CPLStrdup(pszFilename + strlen("NGW:"));
    else if (STARTS_WITH_CI(pszFilename, "NEXTGIS:"))
        sName = CPLStrdup(pszFilename + strlen("NEXTGIS:"));
    else
        sName = CPLStrdup(pszFilename);

    // Initialize NGW reader via HTTP requests. The NGW API version will be determine
    // automatically and NGW handler will do further work according to it.
    NGWWorker *poNgwWorkerTemp = NGWAPI::getWorker(sName.c_str());
    if (poNgwWorkerTemp == NULL)
    {
        CPLError(CE_Failure, CPLE_OpenFailed, "Unable to get NGWWorker. Reason: %s",
                 NGWWorker::getLastMessage().c_str());
        return FALSE;
    }

    // QUESTION: Save some NGW info to the dataset metadata?

    // Get the list of all vector resources.
    json_object *jsonList = poNgwWorkerTemp->getVectorResources();
    if (jsonList == NULL)
    {
        CPLError(CE_Failure, CPLE_OpenFailed, "%s", NGWWorker::getLastMessage().c_str());
        delete poNgwWorkerTemp;
        return FALSE;
    }

    poNgwWorker = poNgwWorkerTemp;

    // Form an array of vector layers.
    int nSize = json_object_array_length(jsonList);
    int nSkippedError = 0;
    int nSkippedNonvec = 0;
    for (int i = 0; i < nSize; i++)
    {
        json_object *jsonRes = json_object_array_get_idx(jsonList, i);

        json_object *jsonType = poNgwWorker->takeResourceType(jsonRes);
        if (jsonType == NULL)
        {
            nSkippedError++;
            continue;
        }
        const char *pszType = json_object_get_string(jsonType);
        if (!poNgwWorker->supportsVectorType(pszType)) // check if this is a vector resource
        {
            nSkippedNonvec++;
            continue;
        }

        json_object *jsonId = poNgwWorker->takeResourceId(jsonRes);
        if (jsonId == NULL)
        {
            nSkippedError++;
            continue;
        }
        int nId = json_object_get_int(jsonId); // note: this will also be the layer's name

        vLayers.push_back(new OGRNGWLayer(this, nId));
    }

    // QUESTION: return FALSE if there are no vector layers in the datasource or leave a
    // void array?

    CPLDebug("NGW", "%d vector layer(s) have been found in the datasource", vLayers.size());
    if (nSkippedNonvec > 0)
        CPLDebug("NGW", "%d non-vector layer(s) skipped", nSkippedNonvec);
    if (nSkippedError > 0)
        CPLDebug("NGW", "%d layer(s) skipped because of errors during parsing of JSON", nSkippedError);

    //json_object_put(jsonList);

    return TRUE;
}


int OGRNGWDataSource::GetLayerCount ()
{
    return vLayers.size();
}


OGRLayer *OGRNGWDataSource::GetLayer (int nLayer)
{
    if (nLayer < 0 || nLayer >= vLayers.size())
        return NULL;

    OGRNGWLayer *poLayer = vLayers[nLayer];
    poLayer->cacheMeta(); // can be a long operation at first

    return poLayer;
}


int OGRNGWDataSource::TestCapability (const char *pszCap)
{
    return FALSE;
}



