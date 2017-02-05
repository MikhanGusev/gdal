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

#ifndef _OGR_NGW_H_INCLUDED
#define _OGR_NGW_H_INCLUDED

#include "ogrsf_frmts.h"
#include "ogr_geojson.h"

#include "ngwapi.h" // the single include of all "NGW-library"

class OGRNGWDataSource;


// Represents a single resource on NextGIS Web server of one of the vector types.
// The name of a layer is NGW ID of the corresponding resource.
class OGRNGWLayer: public OGRLayer
{
    friend class OGRNGWDataSource;

    public:

     OGRNGWLayer (OGRNGWDataSource *poDSIn, int nNgwIdIn);
     ~OGRNGWLayer ();

     void ResetReading ();
     OGRFeature *GetNextFeature ();
     OGRFeatureDefn *GetLayerDefn ();
     int TestCapability (const char *);

    private:

     void cacheMeta ();
     void cacheFeatures ();

     OGRNGWDataSource *poDS;
     OGRFeatureDefn *poFeatureDefn;
     bool bIsMetaCached;
     bool bAreFeaturesCached;
     int nNgwId;
     std::vector<OGRFeature*> vCachedFeatures;
     GIntBig nCurFeature;
};


// Represents a core bunch of resources on NextGIS Web server (i.e. resource with id = 0).
class OGRNGWDataSource: public GDALDataset
{
    friend class OGRNGWLayer; // to use NGWWorker in layers

    public:

     OGRNGWDataSource ();
     ~OGRNGWDataSource ();

     int Open (const char *pszFilename, char **papszOpenOptionsIn, int bUpdateIn = false);
     int GetLayerCount ();
     OGRLayer *GetLayer (int nLayer);
     int TestCapability (const char *pszCap);

    private:

     NGWWorker *poNgwWorker;
     CPLString sName; // base url
     std::vector<OGRNGWLayer*> vLayers;
};


#endif //_OGR_NGW_H_INCLUDED




