/******************************************************************************
 *
 * Project:  GDAL NextGIS Web (NGW) vector-raster driver
 * Purpose:  Main inner include file for NGW API handling
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

#ifndef _NGWAPI_H_INCLUDED
#define _NGWAPI_H_INCLUDED

#include "ogr_json_header.h"
#include "ogrgeojsonreader.h"
#include "cpl_http.h"

struct NGWDateTime
{
    int nYear;
    int nMonth;
    int nDay;
    int nHour;
    int nMinute;
    int nSecond;
};

class NGWWorker;

// Static class with common NGW API methods.
class NGWAPI
{
    public:

     static NGWWorker *getWorker (const char *pszUrl);
};


// Abstract class for working with the specific NGW API version.
// NGWWorker intends to send HTTP requests to the NextGIS Web server and return valid JSONs.
// DEVELOPERS: implement support of different API versions via subclassing this or one of
// the derived classes.
class NGWWorker
{
    public:

     NGWWorker (const char *pszUrl);
     virtual ~NGWWorker ();

     static float getApiVersion (const char *pszUrl);
     static std::string getLastMessage ();
     static json_object *httpGet (const char *pszUrl);
     //static json_object *httpPost ();
     //static json_object *httpPut ();
     //static json_object *httpDelete ();
     static OGRwkbGeometryType geomTypeOgr (const char *pszNgwTypeName);
     static OGRFieldType dataTypeOgr (const char *pszNgwTypeName);

     //virtual bool authenticate (const char *pszLogin, const char *pszPassword);
     virtual bool supportsVectorType (const char *pszType);
     virtual NGWDateTime readDateTime (json_object *jsonObj) = 0;

     //virtual json_object *getUserInfo (int nUserId) = 0;
     //virtual json_object *getSchema () = 0;
     virtual json_object *getVectorResources () = 0;
     virtual json_object *getResourceMeta (int nResourceId) = 0;
     virtual json_object *getResourceFeatures (int nResourceId) = 0;
     //virtual json_object *getFeature (int nResourceId, int nFeatureId) = 0;
     //virtual json_object *getFeatureCount (int nResourceId) = 0;
     //virtual json_object *getGeojson (int nResourceId) = 0;
     //virtual json_object *getCsv (int nResourceId) = 0;

     virtual json_object *takeResourceType (json_object *jsonResourceItem) { return NULL; }
     virtual json_object *takeResourceId (json_object *jsonResourceItem) { return NULL; }
     virtual json_object *takeResourceGeomType (json_object *jsonResource) { return NULL; }
     virtual json_object *takeResourceSrs (json_object *jsonResource) { return NULL; }
     virtual json_object *takeResourceFields (json_object *jsonResource) { return NULL; }
     virtual json_object *takeResourceDispName (json_object *jsonResource) { return NULL; }
     virtual json_object *takeFieldType (json_object *jsonField) { return NULL; }
     virtual json_object *takeFieldName (json_object *jsonField) { return NULL; }
     virtual json_object *takeFeatureId (json_object *jsonFeature) { return NULL; }
     virtual json_object *takeFeatureGeom (json_object *jsonFeature) { return NULL; }
     virtual json_object *takeFeatureAttrs (json_object *jsonFeature) { return NULL; }

    protected:

     static std::string m_sLastMessage;

     static bool _correctJsonObject (json_object *jsonObj);
     static bool _correctJsonArray (json_object *jsonObj);
     static bool _correctJsonString (json_object *jsonObj);
     static bool _correctJsonInt (json_object *jsonObj);

     // Base url with which all http requests will be done.
     std::string m_urlBase;

     // DEVELOPERS: to simply support other versions of API just reassign these variables
     // in the constructors of the derived classes.
     std::set<std::string> m_vSupportedVecTypes;
     //std::string m_urlLogin;
     //std::string m_urlSchema;
     std::string m_urlFlatList;
     std::string m_urlResource;
     std::string m_urlFeatures;
     //std::string m_urlFeature;
     //std::string m_urlFeatureCount;
     //std::string m_urlGeojson;
     //std::string m_urlCsv;

     // FUTURE: store cookies for the authentication.
};


// Supports NGW API of version 3.0.
class NGWWorker30: public NGWWorker
{
    public:

     NGWWorker30 (const char *pszUrl);
     virtual ~NGWWorker30 ();

     virtual NGWDateTime readDateTime (json_object *jsonObj);

     virtual json_object *getVectorResources ();
     virtual json_object *getResourceMeta (int nResourceId);
     virtual json_object *getResourceFeatures (int nResourceId);

     virtual json_object *takeResourceType (json_object *jsonResourceItem);
     virtual json_object *takeResourceId (json_object *jsonResourceItem);
     virtual json_object *takeResourceGeomType (json_object *jsonResource);
     virtual json_object *takeResourceSrs (json_object *jsonResource);
     virtual json_object *takeResourceFields (json_object *jsonResource);
     virtual json_object *takeResourceDispName (json_object *jsonResource);
     virtual json_object *takeFieldType (json_object *jsonField);
     virtual json_object *takeFieldName (json_object *jsonField);
     virtual json_object *takeFeatureId (json_object *jsonFeature);
     virtual json_object *takeFeatureGeom (json_object *jsonFeature);
     virtual json_object *takeFeatureAttrs (json_object *jsonFeature);
};

#endif //_NGWAPI_H_INCLUDED



