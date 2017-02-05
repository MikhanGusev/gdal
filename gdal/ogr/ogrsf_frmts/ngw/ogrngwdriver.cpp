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


static int OGRNGWDriverIdentify (GDALOpenInfo* poOpenInfo)
{
    if (STARTS_WITH_CI(poOpenInfo->pszFilename, "NEXTGISWEB:http://")
        || STARTS_WITH_CI(poOpenInfo->pszFilename, "NEXTGISWEB:https://")
        || STARTS_WITH_CI(poOpenInfo->pszFilename, "NGW:http://")
        || STARTS_WITH_CI(poOpenInfo->pszFilename, "NGW:https://")
        || STARTS_WITH_CI(poOpenInfo->pszFilename, "NEXTGIS:http://")
        || STARTS_WITH_CI(poOpenInfo->pszFilename, "NEXTGIS:https://"))
        return true;
    return false;
}


static GDALDataset *OGRNGWDriverOpen (GDALOpenInfo* poOpenInfo)
{
    if (!OGRNGWDriverIdentify(poOpenInfo))
        return NULL;

    OGRNGWDataSource *poDS = new OGRNGWDataSource();

    if (!poDS->Open(poOpenInfo->pszFilename, poOpenInfo->papszOpenOptions))
    {
        delete poDS;
        poDS = NULL;
    }

    return poDS;
}


static GDALDataset *OGRNGWDriverCreate (const char * pszName, int nBands, int nXSize, int nYSize,
                                        GDALDataType eDT, char **papszOptions)

{
    CPLError(CE_Failure, CPLE_AppDefined,
             "Creation of NextGIS Web resources is temporary unavailable with NGW driver");
    return NULL;
}


void RegisterOGRNGW ()
{
    if (GDALGetDriverByName("NGW") != NULL)
        return;

    GDALDriver *poDriver = new GDALDriver();
    
    poDriver->SetDescription("NGW");
    poDriver->SetMetadataItem(GDAL_DCAP_VECTOR, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "NextGIS Web driver");
    
    poDriver->pfnOpen = OGRNGWDriverOpen;
    poDriver->pfnIdentify = OGRNGWDriverIdentify;
    poDriver->pfnCreate = OGRNGWDriverCreate;
    
    GetGDALDriverManager()->RegisterDriver(poDriver);
}


