//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

#include "PlyFilter.h"

//Local
#include "PlyOpenDlg.h"
#include "ccCoordinatesShiftManager.h"

//Qt
#include <QProgressDialog>
#include <QImage>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

//CCLib
#include <ScalarField.h>

//qCC_db
#include <ccLog.h>
#include <ccMesh.h>
#include <ccPointCloud.h>
#include <ccMaterial.h>
#include <ccMaterialSet.h>

//System
#include <string.h>
#include <assert.h>
#if defined(CC_WINDOWS)
#include <Windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

using namespace CCLib;

CC_FILE_ERROR PlyFilter::saveToFile(ccHObject* entity, const char* filename)
{
	//ask for output format
	QMessageBox msgBox(QMessageBox::Question,"Choose output format","Save in BINARY or ASCII format?");
	msgBox.addButton("BINARY", QMessageBox::AcceptRole);
	QPushButton *asciiButton = msgBox.addButton("ASCII", QMessageBox::AcceptRole);
	msgBox.exec();

	return saveToFile(entity,filename,msgBox.clickedButton() == asciiButton ? PLY_ASCII : PLY_DEFAULT);
}

CC_FILE_ERROR PlyFilter::saveToFile(ccHObject* entity, const char* filename, e_ply_storage_mode storageType)
{
	if (!entity || !filename)
		return CC_FERR_BAD_ARGUMENT;

	ccGenericPointCloud* vertices = NULL;
	ccGenericMesh* mesh = NULL;
	if (entity->isKindOf(CC_TYPES::MESH))
	{
		mesh = ccHObjectCaster::ToGenericMesh(entity);
		vertices = mesh->getAssociatedCloud();
	}
	else if (entity->isKindOf(CC_TYPES::POINT_CLOUD))
	{
		vertices = ccHObjectCaster::ToGenericPointCloud(entity);
	}

	if (!vertices)
		return CC_FERR_BAD_ENTITY_TYPE;

	p_ply ply = ply_create(filename, storageType, NULL, 0, NULL);
	if (!ply)
		return CC_FERR_WRITING;

	//Has the cloud been recentered?
	e_ply_type coordType = vertices->isShifted() || sizeof(PointCoordinateType) > 4 ? PLY_DOUBLE : PLY_FLOAT; //we use double coordinates for shifted vertices (i.e. >1e6)

	int result = 1;
	unsigned vertCount = vertices->size();

	//3D points (x,y,z)
	if (ply_add_element(ply, "vertex", vertCount))
	{
		result = ply_add_scalar_property(ply, "x", coordType);
		result = ply_add_scalar_property(ply, "y", coordType);
		result = ply_add_scalar_property(ply, "z", coordType);
	}
	else result = 0;

	const ccMaterial* material = 0;
	if (mesh && mesh->hasMaterials())
	{
		//look for textures/materials in case there's no color
		//if (!mesh->hasColors())
		{
			unsigned textureCount = 0;
			const ccMaterialSet* materials = mesh->getMaterialSet();
			assert(materials);
			if (materials)
			{
				for (size_t i=0; i < materials->size(); ++i)
				{
					//texture?
					if (materials->at(i).texID != 0)
					{
						//save first encountered texture
						if (!material)
							material = &materials->at(i);
						++textureCount;
					}
				}
			}

			//one texture: we can save it with the PLY file
			if (textureCount == 1)
			{
				if (materials->size() == 1)
				{
					//just one texture/material --> we can handle it 
				}
				else if (materials->size() > 1)
				{
					if (mesh->hasColors())
					{
						ccLog::Error("PLY files can't handle multiple materials/textures!\nOnly the RGB field will be saved.");
					}
					else
					{
						if (mesh->isA(CC_TYPES::MESH))
						{
							if (QMessageBox::question(	0,
														"Multiple materials, one texture",
														"This mesh has only one texture but multiple materials. PLY files can only handle one texture.\nShall we drop the materials (yes) or convert all materials and texture to per-vertex RGB colors? (no)",
														QMessageBox::Yes | QMessageBox::No,
														QMessageBox::Yes) == QMessageBox::No)
							{
								//we can forget the texture
								material = 0;
								//try to convert materials to RGB
								if (!static_cast<ccMesh*>(mesh)->convertMaterialsToVertexColors())
								{
									ccLog::Error("Conversion failed! (not enough memory?)");
								}
							}
						}
						else if (mesh->isA(CC_TYPES::SUB_MESH))
						{
							//we can forget the texture
							material = 0;
							ccLog::Warning("This sub-mesh has one texture and multiple materials. As this is a sub-mesh, we will ignore them...  you should convert the parent mesh textures/materials to RGB colors first");
						}
						else
						{
							assert(false);
						}
					}
				}
				else
				{
					assert(false);
				}
			}
			else if (textureCount > 1) //multiple materials
			{
				assert(materials->size() != 0);
				//we can forget the (first) texture (if any)
				material = 0;

				if (mesh->hasColors())
				{
					ccLog::Error("PLY files can't handle multiple materials/textures!\nOnly the RGB field will be saved.");
				}
				else
				{
					if (mesh->isA(CC_TYPES::MESH))
					{
						//we ask the user if he wants to convert them to RGB
						if (QMessageBox::question(	0,
													"Multiple textures/materials",
													"PLY files can't handle multiple textures/materials!\nDo you want to convert them to per-vertex RGB colors?",
													QMessageBox::Yes | QMessageBox::No,
													QMessageBox::No ) == QMessageBox::Yes)
						{
							if (!static_cast<ccMesh*>(mesh)->convertMaterialsToVertexColors())
							{
								ccLog::Error("Conversion failed! (not enough memory?)");
							}
						}
					}
					else if (mesh->isA(CC_TYPES::SUB_MESH))
					{
						ccLog::Warning("This sub-mesh has multiple textures/materials. PLY files can't handle them.\nAs this is a sub-mesh, we will have to ignore them... you should convert the parent mesh textures/materials to RGB colors first");
					}
					else
					{
						assert(false);
					}
				}
			}
		}
		//else
		//{
		//	ccLog::Warning("[PLY] PLY files can't handle materials/textures! RGB field will be saved instead");
		//	ccLog::Warning("[PLY] Note: you can convert materials/textures to RGB if necessary (see 'Edit > Mesh' menu)");
		//}
	}

	bool hasUniqueColor = false;
	colorType uniqueColor[3]={0,0,0};
	if (material)
	{
		//Material without texture?
		if (material->texture.isNull())
		{
			uniqueColor[0] = (colorType)(material->diffuseFront[0]*MAX_COLOR_COMP);
			uniqueColor[1] = (colorType)(material->diffuseFront[1]*MAX_COLOR_COMP);
			uniqueColor[2] = (colorType)(material->diffuseFront[2]*MAX_COLOR_COMP);
			hasUniqueColor = true;
			material = 0; //we can forget it!
		}
	}

	//RGB colors
	bool hasColors = vertices->hasColors();
	if (hasColors)
	{
		hasColors = true;
		//if (ply_add_element(ply, "color", vertCount))
		//{
			result = ply_add_scalar_property(ply, "red", PLY_UCHAR);
			result = ply_add_scalar_property(ply, "green", PLY_UCHAR);
			result = ply_add_scalar_property(ply, "blue", PLY_UCHAR);
		//}
		//else result = 0;
	}

	//Normals (nx,ny,nz)
	bool hasNormals = vertices->hasNormals();
	if (hasNormals)
	{
		//if (ply_add_element(ply, "normal", vertCount))
		//{
			e_ply_type normType = (sizeof(PointCoordinateType) > 4 ? PLY_DOUBLE : PLY_FLOAT);
			result = ply_add_scalar_property(ply, "nx", normType);
			result = ply_add_scalar_property(ply, "ny", normType);
			result = ply_add_scalar_property(ply, "nz", normType);
		//}
		//else result = 0;
	}

	//Scalar fields
	std::vector<CCLib::ScalarField*> scalarFields;
	if (vertices->isA(CC_TYPES::POINT_CLOUD))
	{
		ccPointCloud* ccCloud = static_cast<ccPointCloud*>(vertices);
		unsigned sfCount = ccCloud->getNumberOfScalarFields();
		if (sfCount)
		{
			e_ply_type scalarType = (sizeof(ScalarType) > 4 ? PLY_DOUBLE : PLY_FLOAT);

			scalarFields.resize(sfCount);
			unsigned unnamedSF = 0;
			for (unsigned i=0; i<sfCount; ++i)
			{
				scalarFields[i] = ccCloud->getScalarField(i);
				const char* sfName = scalarFields[i]->getName();
				QString propName;
				if (!sfName)
				{
					if (unnamedSF++==0)
						propName = "scalar";
					else
						propName = QString("scalar_%1").arg(unnamedSF);
				}
				else
				{
					propName = QString("scalar_%1").arg(sfName);
					propName.replace(' ','_');
				}

				result = ply_add_scalar_property(ply, qPrintable(propName), scalarType);
			}
		}
	}
	
	//Mesh
	unsigned triNum = 0;
	if (mesh)
	{
		triNum = mesh->size();
		if (triNum>0 && ply_add_element(ply, "face", triNum))
		{
			//DGM: don't change the field name (vertex_indices) as Meshlab
			//only support this one! (grrrrrrrrr)
			result = ply_add_list_property(ply, "vertex_indices", PLY_UCHAR, PLY_INT);

			//texture & texture coordinates?
			if (material)
			{
				assert(!material->texture.isNull() && mesh->getTexCoordinatesTable());
				//try to save the texture!
				const QString defaultTextureName("cc_ply_texture.png");
				QString textureFilePath = QFileInfo(filename).absolutePath()+QString('/')+defaultTextureName;
				if (!material->texture.mirrored().save(textureFilePath))
				{
					ccLog::Error("Failed to save texture!");
					material = 0;
				}
				else
				{
					//save texture filename as a comment!
					result = ply_add_comment(ply,qPrintable(QString("TEXTUREFILE %1").arg(defaultTextureName)));
					//DGM FIXME: is this the right name?
					result = ply_add_list_property(ply, "texcoord", PLY_UCHAR, PLY_FLOAT); //'texcoord' to mimick Photoscan
				}
			}
		}
		else result = 0;
	}

	result = ply_add_comment(ply,"Author: CloudCompare (TELECOM PARISTECH/EDF R&D)");
	result = ply_add_obj_info(ply,"Generated by CloudCompare!");

	//try to write header
	result = ply_write_header(ply);
	if (!result)
	{
		ply_close(ply);
		return CC_FERR_WRITING;
	}

	//save the point cloud (=vertices)
	for (unsigned i=0; i<vertCount; ++i)
	{
		const CCVector3* P = vertices->getPoint(i);
		CCVector3d Pglobal = vertices->toGlobal3d<PointCoordinateType>(*P);
		ply_write(ply, Pglobal.x);
		ply_write(ply, Pglobal.y);
		ply_write(ply, Pglobal.z);

		if (hasColors)
		{
			const colorType* col = vertices->getPointColor(i);
			ply_write(ply,static_cast<double>(col[0]));
			ply_write(ply,static_cast<double>(col[1]));
			ply_write(ply,static_cast<double>(col[2]));
		}
		else if (hasUniqueColor)
		{
			ply_write(ply, static_cast<double>(uniqueColor[0]));
			ply_write(ply, static_cast<double>(uniqueColor[1]));
			ply_write(ply, static_cast<double>(uniqueColor[2]));
		}

		if (hasNormals)
		{
			const CCVector3& N = vertices->getPointNormal(i);
			ply_write(ply, static_cast<double>(N.x));
			ply_write(ply, static_cast<double>(N.y));
			ply_write(ply, static_cast<double>(N.z));
		}

		for (std::vector<CCLib::ScalarField*>::const_iterator sf =  scalarFields.begin(); sf != scalarFields.end(); ++sf)
		{
			ply_write(ply, static_cast<double>((*sf)->getValue(i)));
		}
	}

	//and the mesh structure
	if (mesh)
	{
		mesh->placeIteratorAtBegining();
		for (unsigned i=0;i<triNum;++i)
		{
			const CCLib::TriangleSummitsIndexes* tsi = mesh->getNextTriangleIndexes(); //DGM: getNextTriangleIndexes is faster for mesh groups!
			ply_write(ply,double(3));
			assert(tsi->i1<vertCount);
			assert(tsi->i2<vertCount);
			assert(tsi->i3<vertCount);
			ply_write(ply,double(tsi->i1));
			ply_write(ply,double(tsi->i2));
			ply_write(ply,double(tsi->i3));

			if (material) //texture coordinates
			{
				ply_write(ply,double(6));
				float *tx1=0,*tx2=0,*tx3=0;
				mesh->getTriangleTexCoordinates(i,tx1,tx2,tx3);
				ply_write(ply,tx1 ? (double)tx1[0] : -1.0);
				ply_write(ply,tx1 ? (double)tx1[1] : -1.0);
				ply_write(ply,tx2 ? (double)tx2[0] : -1.0);
				ply_write(ply,tx2 ? (double)tx2[1] : -1.0);
				ply_write(ply,tx3 ? (double)tx3[0] : -1.0);
				ply_write(ply,tx3 ? (double)tx3[1] : -1.0);
			}
		}
	}

	ply_close(ply);

	return CC_FERR_NO_ERROR;
}

#define PROCESS_EVENTS_FREQ 10000

#define ELEM_POS_0	0x00000000
#define ELEM_POS_1	0x00000001
#define ELEM_POS_2	0x00000002
#define ELEM_POS_3	0x00000003
#define ELEM_EOL	0x00000004

#define POS_MASK	0x00000003

static double s_Point[3] = {0,0,0};
static int s_PointCount = 0;
static bool s_PointDataCorrupted = false;
static bool s_ShiftAlreadyEnabled = false;
static bool s_AlwaysDisplayLoadDialog = true;
static bool s_ShiftApplyAll = false;
static CCVector3d s_Pshift(0,0,0);

static int vertex_cb(p_ply_argument argument)
{
	long flags;
	ccPointCloud* cloud;
	ply_get_argument_user_data(argument, (void**)(&cloud), &flags);

	double val = ply_get_argument_value(argument);

	// This looks like it should always be true, 
	// but it's false if x is NaN.
	if (val == val)
	{
		s_Point[flags & POS_MASK] = val;
	}
	else
	{
		//warning: corrupted data!
		s_PointDataCorrupted = true;
		s_Point[flags & POS_MASK] = 0;
		//return 0;
	}

	if (flags & ELEM_EOL)
	{
		//first point: check for 'big' coordinates
		if (s_PointCount == 0)
		{
			s_ShiftApplyAll = false; //should already be false!
			if (	sizeof(PointCoordinateType) < 8
				&&	ccCoordinatesShiftManager::Handle(s_Point,0,s_AlwaysDisplayLoadDialog,s_ShiftAlreadyEnabled,s_Pshift,0,s_ShiftApplyAll))
			{
				cloud->setGlobalShift(s_Pshift);
				ccLog::Warning("[PLYFilter::loadFile] Cloud (vertices) has been recentered! Translation: (%.2f,%.2f,%.2f)",s_Pshift.x,s_Pshift.y,s_Pshift.z);
			}
		}

		cloud->addPoint(CCVector3(	static_cast<PointCoordinateType>(s_Point[0]+s_Pshift.u[0]),
									static_cast<PointCoordinateType>(s_Point[1]+s_Pshift.u[1]),
									static_cast<PointCoordinateType>(s_Point[2]+s_Pshift.u[2])));
		++s_PointCount;

		s_PointDataCorrupted = false;
		if ((s_PointCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static CCVector3 s_Normal(0,0,0);
static int s_NormalCount = 0;

static int normal_cb(p_ply_argument argument)
{
	long flags;
	ccPointCloud* cloud;
	ply_get_argument_user_data(argument, (void**)(&cloud), &flags);

	s_Normal.u[flags & POS_MASK] = static_cast<PointCoordinateType>( ply_get_argument_value(argument) );

	if (flags & ELEM_EOL)
	{
		cloud->addNorm(s_Normal);
		++s_NormalCount;

		if ((s_NormalCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static colorType s_color[3]={0,0,0};
static int s_ColorCount=0;

static int rgb_cb(p_ply_argument argument)
{
	long flags;
	ccPointCloud* cloud;
	ply_get_argument_user_data(argument, (void**)(&cloud), &flags);

	p_ply_property prop;
	ply_get_argument_property(argument, &prop, NULL, NULL);
	e_ply_type type;
	ply_get_property_info(prop, NULL, &type, NULL, NULL);

	switch(type)
	{
	case PLY_FLOAT:
	case PLY_DOUBLE:
	case PLY_FLOAT32:
	case PLY_FLOAT64:
		s_color[flags & POS_MASK] = colorType(std::min(std::max(0.0, ply_get_argument_value(argument)), 1.0)*float(MAX_COLOR_COMP));
		break;
	case PLY_INT8:
	case PLY_UINT8:
	case PLY_CHAR:
	case PLY_UCHAR:
		s_color[flags & POS_MASK] = colorType(ply_get_argument_value(argument));
		break;
	default:
		s_color[flags & POS_MASK] = colorType(ply_get_argument_value(argument));
		break;
	}

	if (flags & ELEM_EOL)
	{
		cloud->addRGBColor(s_color);
		++s_ColorCount;

		if ((s_ColorCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static int s_IntensityCount=0;
static int grey_cb(p_ply_argument argument)
{
	ccPointCloud* cloud;
	ply_get_argument_user_data(argument, (void**)(&cloud), NULL);

	p_ply_property prop;
	ply_get_argument_property(argument, &prop, NULL, NULL);
	e_ply_type type;
	ply_get_property_info(prop, NULL, &type, NULL, NULL);

	colorType G;

	switch(type)
	{
	case PLY_FLOAT:
	case PLY_DOUBLE:
	case PLY_FLOAT32:
	case PLY_FLOAT64:
		G = colorType(std::min(std::max(0.0, ply_get_argument_value(argument)), 1.0)*float(MAX_COLOR_COMP));
		break;
	case PLY_INT8:
	case PLY_UINT8:
	case PLY_CHAR:
	case PLY_UCHAR:
		G = colorType(ply_get_argument_value(argument));
		break;
	default:
		G = colorType(ply_get_argument_value(argument));
		break;
	}

	cloud->addGreyColor(G);
	++s_IntensityCount;

	if ((s_IntensityCount % PROCESS_EVENTS_FREQ) == 0)
		QCoreApplication::processEvents();

	return 1;
}

static unsigned s_totalScalarCount = 0;
static int scalar_cb(p_ply_argument argument)
{
	CCLib::ScalarField* sf = 0;
	ply_get_argument_user_data(argument, (void**)(&sf), NULL);

	p_ply_element element;
	long instance_index;
	ply_get_argument_element(argument, &element, &instance_index);

	ScalarType scal = static_cast<ScalarType>(ply_get_argument_value(argument));
	sf->setValue(instance_index,scal);

	if ((++s_totalScalarCount % PROCESS_EVENTS_FREQ) == 0)
		QCoreApplication::processEvents();

	return 1;
}

static unsigned s_tri[3];
static unsigned s_triCount = 0;
static bool s_unsupportedPolygonType = false;
static int face_cb(p_ply_argument argument)
{
	ccMesh* mesh=0;
	ply_get_argument_user_data(argument, (void**)(&mesh), NULL);
	assert(mesh);
	if (!mesh)
		return 1;

	long length, value_index;
	ply_get_argument_property(argument, NULL, &length, &value_index);
	//unsupported polygon type!
	if (length != 3)
	{
		s_unsupportedPolygonType = true;
		return 1;
	}
	if (value_index<0 || value_index>2)
		return 1;

	s_tri[value_index] = (unsigned)ply_get_argument_value(argument);

	if (value_index==2)
	{
		mesh->addTriangle(s_tri[0],s_tri[1],s_tri[2]);
		++s_triCount;

		if ((s_triCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

static float s_texCoord[6];
static unsigned s_texCoordCount = 0;
static bool s_invalidTexCoordinates = false;
static int texCoords_cb(p_ply_argument argument)
{
	TextureCoordsContainer* texCoords=0;
	ply_get_argument_user_data(argument, (void**)(&texCoords), NULL);
	assert(texCoords);
	if (!texCoords)
		return 1;

	long length, value_index;
	ply_get_argument_property(argument, NULL, &length, &value_index);
	//unsupported/invalid coordinates!
	if (length != 6)
	{
		s_invalidTexCoordinates = true;
		return 1;
	}
	if (value_index<0 || value_index>5)
		return 1;

	s_texCoord[value_index] = (float)ply_get_argument_value(argument);

	if (((value_index+1)%2)==0)
	{
		texCoords->addElement(s_texCoord+value_index-1);
		++s_texCoordCount;

		if ((s_texCoordCount % PROCESS_EVENTS_FREQ) == 0)
			QCoreApplication::processEvents();
	}

	return 1;
}

CC_FILE_ERROR PlyFilter::loadFile(const char* filename, ccHObject& container, bool alwaysDisplayLoadDialog/*=true*/, bool* coordinatesShiftEnabled/*=0*/, CCVector3d* coordinatesShift/*=0*/)
{
	//reset statics!
	s_triCount = 0;
	s_unsupportedPolygonType = false;
	s_texCoordCount = 0;
	s_invalidTexCoordinates = false;
	s_totalScalarCount = 0;
	s_IntensityCount = 0;
	s_ColorCount = 0;
	s_NormalCount = 0;
	s_PointCount = 0;
	s_PointDataCorrupted = false;
	s_AlwaysDisplayLoadDialog = alwaysDisplayLoadDialog;
	s_ShiftApplyAll = false;
	s_ShiftAlreadyEnabled = (coordinatesShiftEnabled && *coordinatesShiftEnabled && coordinatesShift);
	if (s_ShiftAlreadyEnabled)
		s_Pshift = *coordinatesShift;
	else
		s_Pshift = CCVector3d(0,0,0);

	/****************/
	/***  Header  ***/
	/****************/

	//open a PLY file for reading
	p_ply ply = ply_open(filename,NULL, 0, NULL);
	if (!ply)
		return CC_FERR_READING;

	ccLog::PrintDebug("[PLY] Opening file '%s' ...",filename);

	if (!ply_read_header(ply))
	{
		ply_close(ply);
		return CC_FERR_WRONG_FILE_TYPE;
	}

	//storage mode: little/big endian
	e_ply_storage_mode storage_mode;
	get_plystorage_mode(ply,&storage_mode);

	/*****************/
	/***  Texture  ***/
	/*****************/
	//eventual texture file declared in the comments (keyword: TEXTUREFILE)
	QString textureFileName;
	//texture coordinates
	TextureCoordsContainer* texCoords = 0;

	/******************/
	/***  Comments  ***/
	/******************/
	{
		const char* lastComment = NULL;

		//display comments
		while ((lastComment = ply_get_next_comment(ply, lastComment)))
		{
			ccLog::Print("[PLY][Comment] %s",lastComment);

			//specific case: TextureFile 'filename.ext'
			if (QString(lastComment).toUpper().startsWith("TEXTUREFILE "))
				textureFileName = QString(lastComment).mid(12).trimmed();
		}
	}

	/*******************************/
	/***  Elements & properties  ***/
	/*******************************/

	//Point-based elements (points, colors, normals, etc.)
	std::vector<plyElement> pointElements;
	//Mesh-based elements (vertices, etc.)
	std::vector<plyElement> meshElements;

	//Point-based element properties (coordinates, color components, etc.)
	std::vector<plyProperty> stdProperties;
	//Mesh-based element properties (vertex indexes, etc.)
	std::vector<plyProperty> listProperties;

	//last read element
	plyElement lastElement;
	lastElement.elem = 0;
	while ((lastElement.elem = ply_get_next_element(ply, lastElement.elem)))
	{
		//we get next element info
		ply_get_element_info(lastElement.elem, &lastElement.elementName, &lastElement.elementInstances);

		if (lastElement.elementInstances == 0)
		{
			ccLog::Warning("[PLY] Element '%s' was ignored as it has 0 instance!",lastElement.elementName);
			continue;
		}

		lastElement.properties.clear();
		lastElement.propertiesCount=0;
		lastElement.isList=false;
		//printf("Element: %s\n",lastElement.elementName);

		//last read property
		plyProperty lastProperty;
		lastProperty.prop = 0;
		lastProperty.elemIndex = 0;

		while ((lastProperty.prop = ply_get_next_property(lastElement.elem,lastProperty.prop)))
		{
			//we get next property info
			ply_get_property_info(lastProperty.prop, &lastProperty.propName, &lastProperty.type, &lastProperty.length_type, &lastProperty.value_type);
			//printf("\tProperty: %s (%s)\n",lastProperty.propName,e_ply_type_names[lastProperty.type]);

			if (lastProperty.type == 16) //PLY_LIST
				lastElement.isList = true;

			lastElement.properties.push_back(lastProperty);
			++lastElement.propertiesCount;
		}

		//if we have a "mesh-like" element
		if (lastElement.isList)
		{
			//we store its properties in 'listProperties'
			for (size_t i=0; i<lastElement.properties.size(); ++i)
			{
				plyProperty& prop = lastElement.properties[i];
				prop.elemIndex = (int)meshElements.size();

				//we only keep track of lists (we can't handle per triangle scalars)
				if (prop.type == 16)
					listProperties.push_back(prop);
				else
				{
					ccLog::Warning("[PLY] Unhandled property: [%s:%s] (%s)",
						lastElement.elementName,
						prop.propName,
						e_ply_type_names[prop.type]);
				}
			}
			meshElements.push_back(lastElement);
		}
		else	//else if we have a "point-like" element
		{
			//we store its properties in 'stdProperties'
			for (size_t i=0; i<lastElement.properties.size(); ++i)
			{
				plyProperty& prop = lastElement.properties[i];
				prop.elemIndex = (int)pointElements.size();
				stdProperties.push_back(prop);
			}
			pointElements.push_back(lastElement);
		}
	}

	//We need some points at least!
	if (pointElements.empty())
	{
		ply_close(ply);
		return CC_FERR_NO_LOAD;
	}

	/**********************/
	/***  Objects info  ***/
	/**********************/
	{
		const char* lastObjInfo = NULL;
		while ((lastObjInfo = ply_get_next_obj_info(ply, lastObjInfo)))
			ccLog::Print("[PLY][Info] %s",lastObjInfo);
	}

	/****************/
	/***  Dialog  ***/
	/****************/

	//properties indexes (0 = unassigned)
	static const unsigned nStdProp = 10;
	int stdPropIndexes[nStdProp] = {0,0,0,0,0,0,0,0,0,0};
	int& xIndex = stdPropIndexes[0];
	int& yIndex = stdPropIndexes[1];
	int& zIndex = stdPropIndexes[2];
	int& nxIndex = stdPropIndexes[3];
	int& nyIndex = stdPropIndexes[4];
	int& nzIndex = stdPropIndexes[5];
	int& rIndex = stdPropIndexes[6];
	int& gIndex = stdPropIndexes[7];
	int& bIndex = stdPropIndexes[8];
	int& iIndex = stdPropIndexes[9];

	std::vector<int> sfPropIndexes;
	//int& sfIndex = stdPropIndexes[10];

	static const unsigned nListProp = 2;
	int listPropIndexes[nListProp] = {0,0};
	int& facesIndex = listPropIndexes[0];
	int& texCoordsIndex = listPropIndexes[1];

	//Combo box items for standard properties (coordinates, color components, etc.)
	QStringList stdPropsText;
	stdPropsText << QString("None");
	{
		for (int i=1; i<=static_cast<int>(stdProperties.size()); ++i)
		{
			plyProperty& pp = stdProperties[i-1];
			QString itemText = QString("%1 - %2 [%3]").arg(pointElements[pp.elemIndex].elementName).arg(pp.propName).arg(e_ply_type_names[pp.type]);
			assert(pp.type!=16); //we don't want any PLY_LIST here
			stdPropsText << itemText;

			QString elementName = QString(pointElements[pp.elemIndex].elementName).toUpper();
			QString propName = QString(pp.propName).toUpper();

			if (nxIndex == 0 && (propName.contains("NX") || (elementName.contains("NORM") && propName.endsWith("X"))))
				nxIndex = i;
			else if (nyIndex == 0 && (propName.contains("NY") || (elementName.contains("NORM") && propName.endsWith("Y"))))
				nyIndex = i;
			else if (nzIndex == 0 && (propName.contains("NZ") || (elementName.contains("NORM") && propName.endsWith("Z"))))
				nzIndex = i;
			else if (rIndex == 0 && (propName.contains("RED") || (elementName.contains("COL") && propName.endsWith("R"))))
				rIndex = i;
			else if (gIndex == 0 && (propName.contains("GREEN") || (elementName.contains("COL") && propName.endsWith("G"))))
				gIndex = i;
			else if (bIndex == 0 && (propName.contains("BLUE") || (elementName.contains("COL") && propName.endsWith("B"))))
				bIndex = i;
			else if (iIndex == 0 && (propName.contains("INTENSITY") || propName.contains("GRAY") || propName.contains("GREY") || (elementName.contains("COL") && propName.endsWith("I"))))
				iIndex = i;
			else if (elementName.contains("VERT") || elementName.contains("POINT"))
			{
				if (propName.contains("SCAL"))
					sfPropIndexes.push_back(i);
				else if (xIndex == 0 && propName.endsWith("X"))
					xIndex = i;
				else if (yIndex == 0 && propName.endsWith("Y"))
					yIndex = i;
				else if (zIndex == 0 && propName.endsWith("Z"))
					zIndex = i;
			}
			else if (propName.contains("SCAL") || propName.contains("VAL"))
				sfPropIndexes.push_back(i);
		}
	}

	//Combo box items for list properties (vertex indexes, etc.)
	QStringList listPropsText;
	{
		listPropsText << QString("None");
		for (int i=0; i<static_cast<int>(listProperties.size()); ++i)
		{
			plyProperty& pp = listProperties[i];
			QString itemText = QString("%0 - %1 [%2]").arg(meshElements[pp.elemIndex].elementName).arg(pp.propName).arg(e_ply_type_names[pp.type]);
			assert(pp.type==16); //we only want PLY_LIST here
			listPropsText << itemText;

			QString elementName = QString(meshElements[pp.elemIndex].elementName).toUpper();
			QString propName = QString(pp.propName).toUpper();

			if (elementName.contains("FACE") || elementName.contains("TRI"))
			{
				if (facesIndex == 0 && propName.contains("IND"))
					facesIndex = i+1;
				if (texCoordsIndex == 0 && propName.contains("COORD"))
					texCoordsIndex = i+1;
			}
		}
	}

	//combo-box max visible items
	int stdPropsCount = stdPropsText.count();
	int listPropsCount = listPropsText.count();

	//we need at least 2 coordinates!
	if (stdPropsCount < 2)
	{
		return CC_FERR_BAD_ENTITY_TYPE;
	}
	else if (stdPropsCount < 4 && !alwaysDisplayLoadDialog)
	{
		//brute force heuristic
		xIndex = 1;
		yIndex = 2;
		zIndex = (stdPropsCount > 3 ? 3 : 0);
		facesIndex = (listPropsCount > 1 ? 1 : 0);
	}
	else
	{
		//we count all assigned properties
		int assignedStdProperties = 0;
		{
			for (unsigned i=0; i<nStdProp; ++i)
				if (stdPropIndexes[i] > 0)
					++assignedStdProperties;
		}

		int assignedListProperties = 0;
		{
			for (unsigned i=0; i<nListProp; ++i)
				if (listPropIndexes[i] > 0)
					++assignedListProperties;
		}

		if (	alwaysDisplayLoadDialog
			||	stdPropsCount > assignedStdProperties+1		//+1 because of the first item in the combo box ('none')
			||	listPropsCount > assignedListProperties+1 )	//+1 because of the first item in the combo box ('none')
		{
			PlyOpenDlg pod/*(MainWindow::TheInstance())*/;
			pod.plyTypeEdit->setText(e_ply_storage_mode_names[storage_mode]);
			pod.elementsEdit->setText(QString::number(pointElements.size()));
			pod.propertiesEdit->setText(QString::number(listProperties.size()+stdProperties.size()));

			//we fill all combo-boxes with all items
			pod.setDefaultComboItems(stdPropsText);
			pod.setListComboItems(listPropsText);

			//Set default/guessed element
			pod.xComboBox->setCurrentIndex(xIndex);
			pod.yComboBox->setCurrentIndex(yIndex);
			pod.zComboBox->setCurrentIndex(zIndex);

			pod.rComboBox->setCurrentIndex(rIndex);
			pod.gComboBox->setCurrentIndex(gIndex);
			pod.bComboBox->setCurrentIndex(bIndex);

			pod.iComboBox->setCurrentIndex(iIndex);

			pod.sfComboBox->setCurrentIndex(sfPropIndexes.empty() ? 0 : sfPropIndexes.front());
			for (size_t j=1; j<sfPropIndexes.size(); ++j)
				pod.addSFComboBox(sfPropIndexes[j]);
			
			pod.nxComboBox->setCurrentIndex(nxIndex);
			pod.nyComboBox->setCurrentIndex(nyIndex);
			pod.nzComboBox->setCurrentIndex(nzIndex);

			pod.facesComboBox->setCurrentIndex(facesIndex);
			pod.textCoordsComboBox->setCurrentIndex(texCoordsIndex);

			//We execute dialog
			if (alwaysDisplayLoadDialog && !pod.exec())
			{
				ply_close(ply);
				return CC_FERR_CANCELED_BY_USER;
			}

			//Force events processing (to hide dialog)
			QCoreApplication::processEvents();

			xIndex = pod.xComboBox->currentIndex();
			yIndex = pod.yComboBox->currentIndex();
			zIndex = pod.zComboBox->currentIndex();
			nxIndex = pod.nxComboBox->currentIndex();
			nyIndex = pod.nyComboBox->currentIndex();
			nzIndex = pod.nzComboBox->currentIndex();
			rIndex = pod.rComboBox->currentIndex();
			gIndex = pod.gComboBox->currentIndex();
			bIndex = pod.bComboBox->currentIndex();
			iIndex = pod.iComboBox->currentIndex();
			facesIndex = pod.facesComboBox->currentIndex();
			texCoordsIndex = pod.textCoordsComboBox->currentIndex();

			//get (non null) SF properties
			sfPropIndexes.clear();
			{
				for (size_t j=0; j<pod.m_sfCombos.size(); ++j)
					if (pod.m_sfCombos[j]->currentIndex() > 0)
						sfPropIndexes.push_back(pod.m_sfCombos[j]->currentIndex());
			}
		}
	}

	/*************************/
	/***  Callbacks setup  ***/
	/*************************/

	//Main point cloud
	ccPointCloud* cloud = new ccPointCloud("unnamed - Cloud");

	/* POINTS (X,Y,Z) */

	unsigned numberOfPoints = 0;

	assert(xIndex != yIndex && xIndex != zIndex && yIndex != zIndex);

	//POINTS (X)
	if (xIndex > 0)
	{
		long flags = ELEM_POS_0; //X coordinate
		if (xIndex > yIndex && xIndex > zIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[xIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, vertex_cb, cloud, flags);

		numberOfPoints = pointElements[pp.elemIndex].elementInstances;
	}

	//POINTS (Y)
	if (yIndex > 0)
	{
		long flags = ELEM_POS_1; //Y coordinate
		if (yIndex > xIndex && yIndex > zIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[yIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, vertex_cb, cloud, flags);

		if (numberOfPoints > 0)
		{
			if ((long)numberOfPoints != pointElements[pp.elemIndex].elementInstances)
			{
				ccLog::Warning("[PLY] Bad/uncompatible assignation of point properties!");
				delete cloud;
				ply_close(ply);
				return CC_FERR_BAD_ENTITY_TYPE;
			}
		}
		else numberOfPoints = pointElements[pp.elemIndex].elementInstances;
	}

	//POINTS (Z)
	if (zIndex > 0)
	{
		long flags = ELEM_POS_2; //Z coordinate
		if (zIndex > xIndex && zIndex > yIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[zIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, vertex_cb, cloud, flags);

		if (numberOfPoints > 0)
		{
			if ((long)numberOfPoints != pointElements[pp.elemIndex].elementInstances)
			{
				ccLog::Warning("[PLY] Bad/uncompatible assignation of point properties!");
				delete cloud;
				ply_close(ply);
				return CC_FERR_BAD_ENTITY_TYPE;
			}
		}
		else numberOfPoints = pointElements[pp.elemIndex].elementInstances;
	}

	if (numberOfPoints == 0 || !cloud->reserveThePointsTable(numberOfPoints))
	{
		delete cloud;
		ply_close(ply);
		return CC_FERR_NOT_ENOUGH_MEMORY;
	}

	/* NORMALS (X,Y,Z) */

	unsigned numberOfNormals=0;

	assert(nxIndex == 0 || (nxIndex != nyIndex && nxIndex != nzIndex));
	assert(nyIndex == 0 || (nyIndex != nxIndex && nyIndex != nzIndex));
	assert(nzIndex == 0 || (nzIndex != nxIndex && nzIndex != nyIndex));

	//NORMALS (X)
	if (nxIndex > 0)
	{
		long flags = ELEM_POS_0; //Nx
		if (nxIndex > nyIndex && nxIndex > nzIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[nxIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, normal_cb, cloud, flags);

		numberOfNormals = pointElements[pp.elemIndex].elementInstances;
	}

	//NORMALS (Y)
	if (nyIndex > 0)
	{
		long flags = ELEM_POS_1; //Ny
		if (nyIndex > nxIndex && nyIndex > nzIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[nyIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, normal_cb, cloud, flags);

		numberOfNormals = std::max(numberOfNormals, (unsigned)pointElements[pp.elemIndex].elementInstances);
	}

	//NORMALS (Z)
	if (nzIndex > 0)
	{
		long flags = ELEM_POS_2; //Nz
		if (nzIndex > nxIndex && nzIndex > nyIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[nzIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, normal_cb, cloud, flags);

		numberOfNormals = std::max(numberOfNormals, (unsigned)pointElements[pp.elemIndex].elementInstances);
	}

	//We check that the number of normals corresponds to the number of points
	if (numberOfNormals > 0)
	{
		if (numberOfPoints != numberOfNormals)
		{
			ccLog::Warning("[PLY] The number of normals doesn't match the number of points!");
			delete cloud;
			ply_close(ply);
			return CC_FERR_BAD_ENTITY_TYPE;
		}
		if (!cloud->reserveTheNormsTable())
		{
			delete cloud;
			ply_close(ply);
			return CC_FERR_NOT_ENOUGH_MEMORY;
		}
		cloud->showNormals(true);
	}

	/* COLORS (R,G,B) */

	unsigned numberOfColors=0;

	assert(rIndex == 0 || (rIndex != gIndex && rIndex != bIndex));
	assert(gIndex == 0 || (gIndex != rIndex && gIndex != bIndex));
	assert(bIndex == 0 || (bIndex != rIndex && bIndex != gIndex));

	if (rIndex > 0)
	{
		long flags = ELEM_POS_0; //R
		if (rIndex > gIndex && rIndex > bIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[rIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, rgb_cb, cloud, flags);

		numberOfColors = pointElements[pp.elemIndex].elementInstances;
	}

	if (gIndex > 0)
	{
		long flags = ELEM_POS_1; //G
		if (gIndex > rIndex && gIndex > bIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[gIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, rgb_cb, cloud, flags);

		numberOfColors = std::max(numberOfColors, (unsigned)pointElements[pp.elemIndex].elementInstances);
	}

	if (bIndex > 0)
	{
		long flags = ELEM_POS_2; //B
		if (bIndex > rIndex && bIndex > gIndex)
			flags |= ELEM_EOL;

		plyProperty& pp = stdProperties[bIndex-1];
		ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, rgb_cb, cloud, flags);

		numberOfColors = std::max(numberOfColors, (unsigned)pointElements[pp.elemIndex].elementInstances);
	}

	/* Intensity (I) */

	//INTENSITE (G)
	if (iIndex > 0)
	{
		if (numberOfColors > 0)
		{
			ccLog::Error("Can't import colors AND intensity (intensities will be ignored)!");
			ccLog::Warning("[PLY] intensities will be ignored");
		}
		else
		{
			plyProperty pp = stdProperties[iIndex-1];
			ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, grey_cb, cloud, 0);

			numberOfColors = pointElements[pp.elemIndex].elementInstances;
		}
	}

	//We check that the number of colors corresponds to the number of points
	if (numberOfColors > 0)
	{
		if (numberOfPoints != numberOfColors)
		{
			ccLog::Warning("The number of colors doesn't match the number of points!");
			delete cloud;
			ply_close(ply);
			return CC_FERR_BAD_ENTITY_TYPE;
		}
		if (!cloud->reserveTheRGBTable())
		{
			delete cloud;
			ply_close(ply);
			return CC_FERR_NOT_ENOUGH_MEMORY;
		}
		cloud->showColors(true);
	}

	/* SCALAR FIELDS (SF) */
	{
		for (size_t i=0; i<sfPropIndexes.size(); ++i)
		{
			int sfIndex = sfPropIndexes[i];
			plyProperty& pp = stdProperties[sfIndex-1];
			
			unsigned numberOfScalars = pointElements[pp.elemIndex].elementInstances;

			//does the number of scalars matches the number of points?
			if (numberOfPoints != numberOfScalars)
			{
				ccLog::Error(QString("Scalar field #%1: the number of scalars doesn't match the number of points (they will be ignored)!").arg(i+1));
				ccLog::Warning(QString("[PLY] Scalar field #%1 ignored!").arg(i+1));
				numberOfScalars = 0;
			}
			else 
			{
				QString qPropName(pp.propName);
				if (qPropName.startsWith("scalar_") && qPropName.length() > 7)
				{
					//remove the 'scalar_' prefix added when saving SF with CC!
					qPropName = qPropName.mid(7).replace('_',' ');
				}

				int sfIdx = cloud->addScalarField(qPrintable(qPropName));
				if (sfIdx >= 0)
				{
					CCLib::ScalarField* sf = cloud->getScalarField(sfIdx);
					assert(sf);
					if (sf->reserve(numberOfScalars))
					{
						ply_set_read_cb(ply, pointElements[pp.elemIndex].elementName, pp.propName, scalar_cb, sf, 1);
					}
					else
					{
						cloud->deleteScalarField(sfIdx);
						sfIdx = -1;
					}
				}
				
				if (sfIdx < 0)
				{
					ccLog::Error(QString("Scalar field #%1: not enough memory to load scalar field (they will be ignored)!").arg(i+1));
					ccLog::Warning(QString("[PLY] Scalar field #%1 ignored!").arg(i+1));
				}
			}
		}
	}

	/* MESH FACETS (TRI) */

	ccMesh* mesh = 0;
	unsigned numberOfFacets = 0;

	if (facesIndex > 0)
	{
		plyProperty& pp = listProperties[facesIndex-1];
		assert(pp.type==16); //we only accept PLY_LIST here!

		mesh = new ccMesh(cloud);

		numberOfFacets = meshElements[pp.elemIndex].elementInstances;

		if (!mesh->reserve(numberOfFacets))
		{
			ccLog::Error("Not enough memory to load facets (they will be ignored)!");
			ccLog::Warning("[PLY] Mesh ignored!");
			delete mesh;
			mesh = 0;
			numberOfFacets = 0;
		}
		else
		{
			ply_set_read_cb(ply, meshElements[pp.elemIndex].elementName, pp.propName, face_cb, mesh, 0);
		}
	}

	if (texCoordsIndex > 0)
	{
		plyProperty& pp = listProperties[texCoordsIndex-1];
		assert(pp.type == 16); //we only accept PLY_LIST here!

		texCoords = new TextureCoordsContainer();
		texCoords->link();

		long numberOfCoordinates = meshElements[pp.elemIndex].elementInstances;
		assert(numberOfCoordinates == numberOfFacets);

		if (!texCoords->reserve(numberOfCoordinates*3))
		{
			ccLog::Error("Not enough memory to load texture coordinates (they will be ignored)!");
			ccLog::Warning("[PLY] Texture coordinates ignored!");
			texCoords->release();
			texCoords = 0;
		}
		else
		{
			ply_set_read_cb(ply, meshElements[pp.elemIndex].elementName, pp.propName, texCoords_cb, texCoords, 0);
		}
	}

	QProgressDialog pDlg(QString("Loading in progress..."),QString(),0,0);
	if (alwaysDisplayLoadDialog)
	{
		pDlg.setWindowTitle("PLY file");
		pDlg.show();
		QApplication::processEvents();
	}

	//let 'Rply' do the job;)
	int success = 0;
	try
	{
		success = ply_read(ply);
	}
	catch(...)
	{
		success = -1;
	}

	ply_close(ply);

	if (success < 1)
	{
		if (mesh)
			delete mesh;
		delete cloud;
		return CC_FERR_READING;
	}

	//we check mesh
	if (mesh && mesh->size() == 0)
	{
		if (s_unsupportedPolygonType)
			ccLog::Error("Mesh is not triangular! (unsupported)");
		else
			ccLog::Error("Mesh is empty!");
		delete mesh;
		mesh=0;
	}

	if (texCoords && (s_invalidTexCoordinates || s_texCoordCount != 3*mesh->size()))
	{
		ccLog::Error("Invalid texture coordinates! (they will be ignored)");
		texCoords->release();
		texCoords=0;
	}

	//we save coordinates shift information
	if (s_ShiftApplyAll && coordinatesShiftEnabled && coordinatesShift)
	{
		*coordinatesShiftEnabled = true;
		*coordinatesShift = s_Pshift;
	}

	//we update scalar field(s)
	{
		for (unsigned i=0; i<cloud->getNumberOfScalarFields(); ++i)
		{
			CCLib::ScalarField* sf = cloud->getScalarField(i);
			assert(sf);
			sf->computeMinAndMax();
			if (i == 0)
			{
				cloud->setCurrentDisplayedScalarField(0);
				cloud->showSF(true);
			}
		}
	}

	if (mesh)
	{
		assert(s_triCount > 0);
		//check number of loaded facets against 'theoretical' number
		if (s_triCount<numberOfFacets)
		{
			mesh->resize(s_triCount);
			ccLog::Warning("[PLY] Missing vertex indexes!");
		}

		//check that vertex indices start at 0
		unsigned minVertIndex=numberOfPoints,maxVertIndex=0;
		for (unsigned i=0;i<s_triCount;++i)
		{
			const CCLib::TriangleSummitsIndexes* tri = mesh->getTriangleIndexes(i);
			if (tri->i1 < minVertIndex)
				minVertIndex = tri->i1;
			else if (tri->i1 > maxVertIndex)
				maxVertIndex = tri->i1;
			if (tri->i2 < minVertIndex)
				minVertIndex = tri->i2;
			else if (tri->i2 > maxVertIndex)
				maxVertIndex = tri->i2;
			if (tri->i3 < minVertIndex)
				minVertIndex = tri->i3;
			else if (tri->i3 > maxVertIndex)
				maxVertIndex = tri->i3;
		}

		if (maxVertIndex>=numberOfPoints)
		{
			if (maxVertIndex == numberOfPoints && minVertIndex > 0)
			{
				ccLog::Warning("[PLY] Vertex indices seem to be shifted (+1)! We will try to 'unshift' indices (otherwise file is corrupted...)");
				for (unsigned i=0;i<s_triCount;++i)
				{
					CCLib::TriangleSummitsIndexes* tri = mesh->getTriangleIndexes(i);
					--tri->i1;
					--tri->i2;
					--tri->i3;
				}
			}
			else //file is definitely corrupted!
			{
				ccLog::Warning("[PLY] Invalid vertex indices!");
				delete mesh;
				delete cloud;
				return CC_FERR_MALFORMED_FILE;
			}
		}

		mesh->addChild(cloud);
		cloud->setEnabled(false);
		cloud->setName("Vertices");
		//cloud->setLocked(true); //DGM: no need to lock it as it is only used by one mesh!

		//associated texture
		if (texCoords)
		{
			if (!textureFileName.isEmpty())
			{
				QString textureFilePath = QFileInfo(filename).absolutePath()+QString('/')+textureFileName;
				QImage texture = QImage(textureFilePath).mirrored();
				if (!texture.isNull())
				{
					if (mesh->reservePerTriangleTexCoordIndexes() && mesh->reservePerTriangleMtlIndexes())
					{
						ccLog::Print(QString("[PLY][Texture] Successfully loaded texture '%1' (%2x%3 pixels)").arg(textureFileName).arg(texture.width()).arg(texture.height()));
						//materials
						ccMaterialSet* materials = new ccMaterialSet("materials");
						ccMaterial material(textureFileName);
						material.texture = texture;
						memcpy(material.specular,ccColor::bright,sizeof(float)*4);
						memcpy(material.ambient,ccColor::bright,sizeof(float)*4);
						materials->push_back(material);
						mesh->setMaterialSet(materials);
						mesh->setTexCoordinatesTable(texCoords);
						mesh->addChild(texCoords);
						for (unsigned i=0;i<mesh->size();++i)
						{
							mesh->addTriangleMtlIndex(0);
							mesh->addTriangleTexCoordIndexes(i*3,i*3+1,i*3+2);
						}
						mesh->showMaterials(true);
						mesh->addChild(materials);
					}
					else
					{
						ccLog::Warning("[PLY][Texture] Failed to reserve per-triangle texture coordinates! (not enough memory?)");
					}
				}
				else
				{
					ccLog::Warning(QString("[PLY][Texture] Failed to load texture '%1'").arg(textureFilePath));
				}
			}
			else
			{
				ccLog::Warning("[PLY][Texture] Texture coordinates loaded without an associated image! (we look for the 'TextureFile' keyword in comments)");
			}
		}

		if (cloud->hasColors())
			mesh->showColors(true);
		if (cloud->hasDisplayedScalarField())
			mesh->showSF(true);
		if (cloud->hasNormals())
			mesh->showNormals(true);
		else
		{
			//DGM: normals can be per-vertex or per-triangle so it's better to let the user do it himself later
			//Moreover it's not always good idea if the user doesn't want normals (especially in ccViewer!)
			//mesh->computeNormals();
			ccLog::Warning("[PLY] Mesh has no normal! You can manually compute them (select it then call \"Edit > Normals > Compute\")");
		}

		if (mesh->hasMaterials())
			mesh->showNormals(false);

		container.addChild(mesh);
	}
	else
	{
		container.addChild(cloud);
	}

	if (texCoords)
	{
		texCoords->release();
		texCoords = 0;
	}

	return CC_FERR_NO_ERROR;
}
