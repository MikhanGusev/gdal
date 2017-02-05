

include ../../../GDALmake.opt

OBJ	=	ngwworker.o ngwworker30.o ogrngwdriver.o ogrngwdatasource.o ogrngwlayer.o

CPPFLAGS	:=	-I../geojson $(JSON_INCLUDE) -I.. -I../..

default:	$(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

$(O_OBJ):	ogr_ngw.h
