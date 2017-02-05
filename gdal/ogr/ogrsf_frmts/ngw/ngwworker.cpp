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

#define NGW_URL_VERSION "/api/component/pyramid/pkg_version" // fixed url to get versions
#define NGW_JSON_VERSION_API "nextgisweb"


// Common method to get NGW worker of appropriate API version.
// It is a caller's responsibility to delete returned object.
NGWWorker* NGWAPI::getWorker (const char *pszUrl)
{
    // Perform HTTP request to get API version.
    float fVers = NGWWorker::getApiVersion(pszUrl);
    if (fVers == 0.0)
        return NULL;

    // DEVELOPERS: add workers here so the NGW driver can dynamically support different
    // versions of API.

    // By default return worker with API version = 3.0.
    return new NGWWorker30(pszUrl);
}


NGWWorker::NGWWorker (const char *pszUrl)
{
    m_urlBase = pszUrl;

    // QUESTION: maybe form this array after doing schema request to the server and not
    // in constructor.
    m_vSupportedVecTypes.insert("vector_layer");
    //m_vSupportedVecTypes.push_back("postgis_layer"),
    //m_vSupportedVecTypes.push_back("wfsserver_service");
}

NGWWorker::~NGWWorker ()
{

}


// Global method to get API version in a fixed way.
float NGWWorker::getApiVersion (const char *pszUrl) // STATIC
{
    // Determine API version sending according http request.
    std::string urlVers = pszUrl;
    urlVers += NGW_URL_VERSION;
    json_object *jsonVers = NGWWorker::httpGet(urlVers.c_str());
    if (!_correctJsonObject(jsonVers))
    {
//        json_object_put(jsonVers);
        m_sLastMessage = "Incorrect JSON structure of server reply (no NGW versions object)";
        return 0.0;
    }

    json_object* jsonApiVers = CPL_json_object_object_get(jsonVers, NGW_JSON_VERSION_API);
    if (!_correctJsonString(jsonApiVers))
    {
//        json_object_put(jsonVers);
        m_sLastMessage = "Incorrect JSON structure of server reply (no NGW API version)";
        return 0.0;
    }

//    json_object_put(jsonVers);

    // TODO: ideally we can additionally check 404 error here for the cases when user tries to open
    // not the root NGW resource (e.g. http://mikhail.nextgis.com) which is equal to GDALDataset,
    // but the particular resource (e.g. http://mikhail.nextgis.com/resource/13) which is equal
    // to OGRLayer. We can warn user about it.

    const char *pszApiVers = json_object_get_string(jsonApiVers);
    return CPLAtof(pszApiVers); // with '.' delimeter by default
}

std::string NGWWorker::m_sLastMessage;

// Copy last message before returning and clear it.
std::string NGWWorker::getLastMessage () // STATIC
{
    std::string sLastMessage = m_sLastMessage;
    m_sLastMessage.clear();
    return sLastMessage;
}


// Perform HTTP GET request to the server and return correctly-parsed JSON object.
json_object *NGWWorker::httpGet (const char *pszUrl) // STATIC
{
    if (pszUrl == NULL)
        return NULL;

    CPLHTTPResult *httpResult = CPLHTTPFetch(pszUrl, NULL);
    if (httpResult->nStatus != 0)
    {
        m_sLastMessage = "cURL error code: %d"; // httpResult->nStatus
        CPLHTTPDestroyResult(httpResult);
        return NULL;
    }
    if (httpResult->pszErrBuf != NULL)
    {
        m_sLastMessage = "Error message from cURL: %s"; // httpResult->pszErrBuf
        CPLHTTPDestroyResult(httpResult);
        return NULL;
    }
    if (httpResult->pabyData == NULL)
    {
        m_sLastMessage = "Incorrect reply from server. No JSON received";
        CPLHTTPDestroyResult(httpResult);
        return NULL;
    }

    // TODO: Check if this is a common authorization error and write according message.

    // Return a correct json-c object.
    json_tokener *jsonTok = NULL;
    json_object *jsonObj = NULL;
    jsonTok = json_tokener_new();
    jsonObj = json_tokener_parse_ex(jsonTok, (const char*)httpResult->pabyData, -1);
    if (jsonTok->err != json_tokener_success)
    {
        m_sLastMessage = "Unable to parse JSON reply";
        json_tokener_free(jsonTok);
        CPLHTTPDestroyResult(httpResult);
        return NULL;
    }
    json_tokener_free(jsonTok);
    CPLHTTPDestroyResult(httpResult);
    if (jsonObj == NULL)
    {
        m_sLastMessage = "Unable to parse JSON reply";
//        json_object_put(jsonObj);
        return NULL;
    }

    return jsonObj;
}


OGRwkbGeometryType NGWWorker::geomTypeOgr (const char *pszNgwTypeName)
{
    if (strcmp(pszNgwTypeName,"POINT") == 0)
        return wkbPoint;
    if (strcmp(pszNgwTypeName,"LINESTRING") == 0)
        return wkbLineString;
    if (strcmp(pszNgwTypeName,"POLYGON") == 0)
        return wkbPolygon;
    if (strcmp(pszNgwTypeName,"MULTIPOINT") == 0)
        return wkbMultiPoint;
    if (strcmp(pszNgwTypeName,"MULTILINESTRING") == 0)
        return wkbMultiLineString;
    if (strcmp(pszNgwTypeName,"MULTIPOLYGON") == 0)
        return wkbMultiPolygon;
    return wkbUnknown;
}

OGRFieldType NGWWorker::dataTypeOgr (const char *pszNgwTypeName)
{
    if (strcmp(pszNgwTypeName,"STRING") == 0)
        return OFTString;
    if (strcmp(pszNgwTypeName,"INTEGER") == 0)
        return OFTInteger64; // maximum integer size so not to cut NGW integers
    if (strcmp(pszNgwTypeName,"REAL") == 0)
        return OFTReal;
    if (strcmp(pszNgwTypeName,"DATE") == 0)
        return OFTDate;
    if (strcmp(pszNgwTypeName,"TIME") == 0)
        return OFTTime;
    if (strcmp(pszNgwTypeName,"DATETIME") == 0)
        return OFTDateTime;
    return OFTString;
}


bool NGWWorker::supportsVectorType (const char *pszType)
{
    if (m_vSupportedVecTypes.count(pszType) == 1) // all values are unique
        return true;
    return false;
}


// Common actions for checking JSON objects.
// QUESTION: implement as macros?
bool NGWWorker::_correctJsonObject (json_object *jsonObj)
{
    if (jsonObj == NULL
        || json_object_get_type(jsonObj) != json_type_object)
        return false;
    return true;
}
bool NGWWorker::_correctJsonArray (json_object *jsonObj)
{
    if (jsonObj == NULL
        || json_object_get_type(jsonObj) != json_type_array
        || json_object_array_length(jsonObj) == 0)
        return false;
    return true;
}
bool NGWWorker::_correctJsonString (json_object *jsonObj)
{
    if (jsonObj == NULL
        || json_object_get_type(jsonObj) != json_type_string)
        return false;
    return true;
}
bool NGWWorker::_correctJsonInt (json_object *jsonObj)
{
    if (jsonObj == NULL
        || json_object_get_type(jsonObj) != json_type_int)
        return false;
    return true;
}



