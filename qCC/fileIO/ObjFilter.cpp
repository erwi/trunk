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
//
//*********************** Last revision of this file ***********************
//$Author:: dgm                                                            $
//$Rev:: 2276                                                              $
//$LastChangedDate:: 2012-10-18 14:58:26 +0200 (jeu., 18 oct. 2012)        $
//**************************************************************************
//
#include "ObjFilter.h"
#include "../ccCoordinatesShiftManager.h"

//Qt
#include <QApplication>
#include <QFileInfo>
#include <QImage>

//qCC_db
#include <ccMesh.h>
#include <ccMeshGroup.h>
#include <ccMaterial.h>
#include <ccPointCloud.h>
#include <ccProgressDialog.h>
#include <ccNormalVectors.h>
#include <ccMaterialSet.h>

#include "../ccConsole.h"

//! Space char. ASCII code
#ifndef SPACE_ASCII_CODE
#define SPACE_ASCII_CODE 32
#endif
//! Tab char. ASCII code
#ifndef TAB_ASCII_CODE
#define TAB_ASCII_CODE 9
#endif
//! Return char. ASCII code
#ifndef ENTER_ASCII_CODE
#define ENTER_ASCII_CODE 10
#endif

CC_FILE_ERROR ObjFilter::saveToFile(ccHObject* entity, const char* filename)
{
	FILE* theFile = fopen(filename,"wb");

	if (!theFile)
		return CC_FERR_WRITING;

	CC_FILE_ERROR result = saveToFile(entity, theFile);

	fclose(theFile);

	return result;
}

CC_FILE_ERROR ObjFilter::saveToFile(ccHObject* entity, FILE *theFile, unsigned basePointNumber/*=0*/)
{
	if (!theFile || !entity)
		return CC_FERR_BAD_ARGUMENT;

	if (!entity->isKindOf(CC_MESH))
		return CC_FERR_BAD_ENTITY_TYPE;

	ccGenericMesh* mesh = static_cast<ccGenericMesh*>(entity);
	unsigned numberOfTriangles = mesh->size();
	if (numberOfTriangles==0)
	{
		ccConsole::Warning("No facet in mesh %s !",mesh->getName());
		return CC_FERR_NO_ERROR;
	}

	//avancement de la sauvegarde
	ccProgressDialog pdlg(true);
	CCLib::NormalizedProgress nprogress(&pdlg,numberOfTriangles);
	pdlg.setMethodTitle(qPrintable(QString("Saving mesh [%1]").arg(mesh->getName())));
	pdlg.setInfo(qPrintable(QString("Number of facets: %1").arg(numberOfTriangles)));
	pdlg.start();

	if (fprintf(theFile,"#OBJ Generated by CloudCompare (TELECOM PARISTECH/EDF R&D)\n") < 0)
		return CC_FERR_WRITING;

	//vertices
	ccGenericPointCloud* vertices = mesh->getAssociatedCloud();
	unsigned nbPoints = vertices->size();
	const double* shift = vertices->getOriginalShift();
	unsigned i=0;
	for (i=0;i<nbPoints;++i)
	{
		const CCVector3* P = vertices->getPoint(i);
		if (fprintf(theFile,"v %f %f %f\n",-shift[0]+(double)P->x,
											-shift[1]+(double)P->y,
											-shift[2]+(double)P->z) < 0)
			return CC_FERR_WRITING;
	}

	//vertices normals
	bool withTriNormals = mesh->hasTriNormals();
	if (withTriNormals)
	{
		NormsIndexesTableType* normsTable = mesh->getTriNormsTable();
		assert(normsTable);
		for (i=0;i<normsTable->currentSize();++i)
		{
			const PointCoordinateType* _normalVec = ccNormalVectors::GetNormal(normsTable->getValue(i));
			if (fprintf(theFile,"vn %f %f %f\n",_normalVec[0],_normalVec[1],_normalVec[2]) < 0)
				return CC_FERR_WRITING;
		}
	}
	
	//if mesh hasn't per-triangle normals, we try per-vertices ones
	bool withVertNormals = vertices->hasNormals();
	if (!withTriNormals && withVertNormals)
	{
		for (i=0;i<nbPoints;++i)
		{
			const PointCoordinateType* _normalVec = vertices->getPointNormal(i);
			if (fprintf(theFile,"vn %f %f %f\n",_normalVec[0],_normalVec[1],_normalVec[2]) < 0)
				return CC_FERR_WRITING;
		}
	}

	bool withNormals = (withTriNormals || withVertNormals);

	std::vector<ccGenericMesh*> subMeshes;
	subMeshes.push_back(mesh);

	//mesh or sub-meshes
	CCLib::TriangleSummitsIndexes* tsi = NULL;
	while (!subMeshes.empty())
	{
		ccGenericMesh* subMesh = subMeshes.back();
		subMeshes.pop_back();

		if (subMesh->isA(CC_MESH_GROUP))
		{
			for (i=0;i<subMesh->getChildrenNumber();++i)
			{
				ccHObject* child = subMesh->getChild(i);
				if (child->isKindOf(CC_MESH))
					subMeshes.push_back(static_cast<ccGenericMesh*>(child));
			}
		}
		else if (subMesh->isA(CC_MESH))
		{
			ccMesh* st = static_cast<ccMesh*>(subMesh);
			if (fprintf(theFile,"g %s\n",st->getName()) < 0)
				return CC_FERR_WRITING;

			unsigned triNum = st->size();
			st->placeIteratorAtBegining();

			for (i=0;i<triNum;++i)
			{
				tsi = st->getNextTriangleIndexes();
				//for per-triangle normals
				unsigned i1 = (basePointNumber+1)+tsi->i1;
				unsigned i2 = (basePointNumber+1)+tsi->i2;
				unsigned i3 = (basePointNumber+1)+tsi->i3;
				if (withNormals)
				{
					if (fprintf(theFile,"f %i//%i %i//%i %i//%i\n",i1,i1,i2,i2,i3,i3) < 0)
						return CC_FERR_WRITING;
				}
				else
				{
					if (fprintf(theFile,"f %i %i %i\n",i1,i2,i3) < 0)
						return CC_FERR_WRITING;
				}

				if (!nprogress.oneStep()) //cancel requested
					return CC_FERR_CANCELED_BY_USER;
			}

			if (fprintf(theFile,"#%i faces\n",triNum) < 0)
				return CC_FERR_WRITING;
		}
	}

	return CC_FERR_NO_ERROR;
}

//! OBJ facet ('f') element
struct facetElement
{
	//! A set of indexes (vertex, texture coordinates and normal)
	union
	{
		struct
		{
			int vIndex;		//vertex index
			int tcIndex;	//texture coordinate index
			int nIndex;		//normal index
		};
		int indexes[3];
	};

	//! Default constructor
	facetElement()
		: vIndex(0)
		, tcIndex(0)
		, nIndex(0)
	{
	}

	//! Updates point index to a global index starting from 0!
	bool updatePointIndex(int maxIndex)
	{
		if (vIndex == 0 || -vIndex>maxIndex)
			return false;
		vIndex = (vIndex>0 ? vIndex-1 : maxIndex+vIndex);
		return true;
	}

	//! Updates tex coord index to a global index starting from 0!
	bool updateTexCoordIndex(int maxIndex)
	{
		if (tcIndex == 0 || -tcIndex>maxIndex)
			return false;
		tcIndex = (tcIndex>0 ? tcIndex-1 : maxIndex+tcIndex);
		return true;
	}

	//! Updates normal index to a global index starting from 0!
	bool updateNormalIndex(int maxIndex)
	{
		if (nIndex == 0 || -nIndex>maxIndex)
			return false;
		nIndex = (nIndex>0 ? nIndex-1 : maxIndex+nIndex);
		return true;
	}
};

#define OBJ_LOADER_WHITESPACE " \t\n\r"

CC_FILE_ERROR ObjFilter::loadFile(const char* filename, ccHObject& container, bool alwaysDisplayLoadDialog/*=true*/, bool* coordinatesShiftEnabled/*=0*/, double* coordinatesShift/*=0*/)
{
	ccConsole::Print("[ObjFilter::Load] %s",filename);

	//ouverture du fichier
	FILE *fp = fopen(filename, "rt");
	if (!fp)
		return CC_FERR_READING;

	//buffers
	char line[MAX_ASCII_FILE_LINE_LENGTH];

	//current vertex
	CCVector3 P;
	double Pd[3]={0.0,0.0,0.0};
	double Pshift[3]={0.0,0.0,0.0};

	//current normal
	CCVector3 N;
	//current mesh
	ccMesh* tri = 0;
	//current vertex texture
	float T[2];
	//all loaded (sub)meshes
	std::vector<ccMesh*> meshes;

	//facets
	int facesRead = 0;
	int totalFacesRead = 0;
	int maxFaces = 0;
	int maxVertexIndex = -1;
	std::vector<facetElement> currentFace;

	//vertices
	ccPointCloud* vertices = new ccPointCloud("vertices");
	int pointsRead = 0;
	int maxPoints = 0;

	//materials
	ccMaterialSet* materials = 0;
	bool hasMaterial = false;
	int currentMaterial = -1;
	bool currentMaterialDefined=false;
	bool materialsLoadFailed=false;

	//texture coordinates
	TextureCoordsContainer* texCoords = 0;
	bool hasTexCoords = false;
	int texCoordsRead = 0;
	int maxTexCoords = 0;
	int maxTexCoordIndex = -1;

	//normals
	NormsIndexesTableType* normals = 0;
	int normsRead = 0;
	int maxNorms = 0;
	bool normalsPerFacetGlobal = false;
	bool normalsPerFacet = false;
	int maxTriNormIndex = -1;

	//new group name
	char objectName[1024]="default";

	//progress dialog
	ccProgressDialog progressDlg(false);
	progressDlg.setMethodTitle("Loading OBJ file");
	progressDlg.setInfo("Loading in progress...");
	progressDlg.setRange(0,0);
	progressDlg.show();
	QApplication::processEvents();

	//State machine
	bool saveCurrentGroup = false;
	bool createNewGroup = false;
	bool scanLine = false;
	bool restart = false;
	bool stop = false;
	bool error = false;

	//common warnings that can appear multiple time (we avoid to send too many messages to the console!)
	enum OBJ_WARNINGS {		DISCARED_GROUP = 0,
							EMPTY_GROUP = 1,
							INVALID_NORMALS = 2,
							INVALID_INDEX = 3,
							NOT_ENOUGH_MEMORY = 4
	};
	const unsigned objWarningsCount = 5;
	bool objWarnings[objWarningsCount];
	memset(objWarnings,0,sizeof(bool)*objWarningsCount);

	int lineCount=0;
	const char* current_token=0;

	while (!stop)
	{
		//if we don't need to rescan last line
		if (!restart)
		{
			scanLine = (fgets (line , MAX_ASCII_FILE_LINE_LENGTH , fp) > 0);

			//no more line in file?
			if (!scanLine)
			{
				//we save current group and that's it!
				saveCurrentGroup = true;
				stop = true;
			}
			else
			{
				if ((++lineCount % 4096) == 0)
					progressDlg.update(lineCount>>12);

				current_token = strtok( line, OBJ_LOADER_WHITESPACE);

				//skip comments & empty lines
				if( !current_token || current_token[0]=='/' || current_token[0]=='#')
					continue;
		}
		}
		else
		{
			restart = false;
		}

		//standard line scan
		if (scanLine)
		{
			if (strcmp(current_token, "v")==0) //v = vertex
			{
				if (pointsRead >= maxPoints)
				{
					maxPoints += MAX_NUMBER_OF_ELEMENTS_PER_CHUNK;
					if (!vertices->reserve(maxPoints))
					{
						objWarnings[NOT_ENOUGH_MEMORY]=true;
						error=true;
						break;
					}
				}

				Pd[0] = atof( strtok(NULL, OBJ_LOADER_WHITESPACE));
				Pd[1] = atof( strtok(NULL, OBJ_LOADER_WHITESPACE));
				Pd[2] = atof( strtok(NULL, OBJ_LOADER_WHITESPACE));

				//first point: check for 'big' coordinates
				if (pointsRead==0)
				{
					bool shiftAlreadyEnabled = (coordinatesShiftEnabled && *coordinatesShiftEnabled && coordinatesShift);
					if (shiftAlreadyEnabled)
						memcpy(Pshift,coordinatesShift,sizeof(double)*3);
					bool applyAll=false;
					if (ccCoordinatesShiftManager::Handle(Pd,0,alwaysDisplayLoadDialog,shiftAlreadyEnabled,Pshift,0,applyAll))
					{
						vertices->setOriginalShift(Pshift[0],Pshift[1],Pshift[2]);
						ccConsole::Warning("[ObjFilter::loadFile] Cloud has been recentered! Translation: (%.2f,%.2f,%.2f)",Pshift[0],Pshift[1],Pshift[2]);

						//we save coordinates shift information
						if (applyAll && coordinatesShiftEnabled && coordinatesShift)
						{
							*coordinatesShiftEnabled = true;
							coordinatesShift[0] = Pshift[0];
							coordinatesShift[1] = Pshift[1];
							coordinatesShift[2] = Pshift[2];
						}
					}
				}

				P.x = (float)(Pd[0]+Pshift[0]);
				P.y = (float)(Pd[1]+Pshift[1]);
				P.z = (float)(Pd[2]+Pshift[2]);

				vertices->addPoint(P);
				++pointsRead;
			}
			else if (strcmp(current_token, "vt")==0) //vt = vertex texture
			{
				if (texCoordsRead==maxTexCoords)
				{
					if (!texCoords)
					{
						texCoords = new TextureCoordsContainer();
						texCoords->link();
					}

					maxTexCoords += MAX_NUMBER_OF_ELEMENTS_PER_CHUNK;
					if (!texCoords->reserve(maxTexCoords))
					{
						objWarnings[NOT_ENOUGH_MEMORY]=true;
						error=true;
						break;
					}
				}

				T[0] = (float)atof( strtok(NULL, OBJ_LOADER_WHITESPACE));
				T[1] = (float)atof( strtok(NULL, OBJ_LOADER_WHITESPACE));

				texCoords->addElement(T);
				++texCoordsRead;
				//*/
			}
			else if (strcmp(current_token, "vn")==0) //vn = vertex normal --> in fact it can also be a facet normal!!!
			{
				if (normsRead == maxNorms)
					{
					if (!normals)
					{
						normals = new NormsIndexesTableType;
						normals->link();
					}

					maxNorms += MAX_NUMBER_OF_ELEMENTS_PER_CHUNK;
					if (!normals->reserve(maxNorms))
					{
						objWarnings[NOT_ENOUGH_MEMORY]=true;
						error=true;
						break;
					}
				}

				N.x = (float)atof( strtok(NULL, OBJ_LOADER_WHITESPACE));
				N.y = (float)atof( strtok(NULL, OBJ_LOADER_WHITESPACE));
				N.z = (float)atof( strtok(NULL, OBJ_LOADER_WHITESPACE));

				if (fabs(N.norm2() - 1.0)<0.05)
					objWarnings[INVALID_NORMALS]=true;
				normsType nIndex = ccNormalVectors::GetNormIndex(N.u);

				normals->addElement(nIndex); //we don't know yet if it's per-vertex or per-triangle normal...
				++normsRead;
				//*/
			}
			else if (strcmp(current_token, "g")==0) //new group
			{
				saveCurrentGroup = true;
				createNewGroup = true;
				//we get the object name
				const char* groupName = strtok(NULL, "\n\r");
				strcpy(objectName,groupName ? groupName : "default");
			}
			else if (line[0]=='f') //new facet
			{
				if (!tri)
				{
					restart = true;
					createNewGroup = true;
					//we get the object name
					strcpy(objectName,"default");
				}
				else
				{
					//we reset current facet 'state'
					currentFace.clear(); //current face element

					while ((current_token = strtok( 0, OBJ_LOADER_WHITESPACE)) && !error)
					{
						//new summit
						facetElement fe; //(0,0,0) by default
						const char* _char = current_token; //pointer on current character
						int slashes=0;

						//we split the token into groups of '/' (we can't use strtok again!!!)
						while (*_char)
						{
							if (*_char=='/')
							{
								//delimiter
								++slashes;
								if (slashes>2)
								{
									ccConsole::Error("Malformed file: more than 3 slahses ('/') in the same face block (line %i)!",lineCount);
									error=true;
									break;
								}
							}
							else
							{
								//read next element (vert/tex coord/normal)
								sscanf(_char,"%i",fe.indexes+slashes);
								while (_char[1] != 0 && _char[1] !='/')
									++_char;
							}
							++_char;
						}

						currentFace.push_back(fe);
					}

					if (error)
						break;

					unsigned vCount = currentFace.size();
					if (vCount<3)
					{
						ccConsole::Error("Malformed file: face on line %1 has less than 3 vertices!",lineCount);
						error=true;
						break;
					}

					//first vertex
					std::vector<facetElement>::iterator A = currentFace.begin();

					//the very first vertex tells us about the whole sequence
					if (facesRead == 0)
					{
						//we have a tex. coord index as second vertex element!
						if (A->tcIndex != 0 && !materialsLoadFailed)
						{
							if (!tri->reservePerTriangleTexCoordIndexes())
							{
								objWarnings[NOT_ENOUGH_MEMORY]=true;
								error=true;
								break;
							}
							hasTexCoords = true;
						}

						//we have a normal index as third vertex element!
						if (A->nIndex != 0)
						{
							//so the normals are 'per-facet'
							normalsPerFacet = true;
							if (!tri->reservePerTriangleNormalIndexes())
							{
								objWarnings[NOT_ENOUGH_MEMORY]=true;
								error=true;
								break;
							}
						}
					}

					//we process all vertices accordingly
					std::vector<facetElement>::iterator it = currentFace.begin();
					for (;it!=currentFace.end();++it)
					{
						if (!it->updatePointIndex(pointsRead))
						{
							objWarnings[INVALID_INDEX]=true;
							error=true;
							break;
						}
						if (it->vIndex > maxVertexIndex)
							maxVertexIndex = it->vIndex;

						//should we have a tex. coord index as second vertex element?
						if (hasTexCoords)
						{
							if (!it->updateTexCoordIndex(texCoordsRead))
							{
								objWarnings[INVALID_INDEX]=true;
								error=true;
								break;
							}
							if (it->tcIndex>maxTexCoordIndex)
								maxTexCoordIndex = it->tcIndex;
						}

						//should we have a normal index as third vertex element?
						if (normalsPerFacet)
						{
							if (!it->updateNormalIndex(normsRead))
							{
								objWarnings[INVALID_INDEX]=true;
								error=true;
								break;
							}
							if (it->nIndex>maxTriNormIndex)
								maxTriNormIndex = it->nIndex;
						}
					}

					//don't forget material (common for all vertices)
					if (currentMaterialDefined && !materialsLoadFailed)
					{
						if (!hasMaterial)
						{
							//We hope it's the first facet!!!
							assert(facesRead == 0);
							if (!tri->reservePerTriangleMtlIndexes())
							{
								objWarnings[NOT_ENOUGH_MEMORY]=true;
								error=true;
								break;
							}
							hasMaterial = true;
						}
					}

					if (error)
						break;

					//Now, let's tesselate the whole polygon
					//yeah, we do very ulgy tesselation here!
					std::vector<facetElement>::const_iterator B = A+1;
					std::vector<facetElement>::const_iterator C = B+1;
					for (;C != currentFace.end();++B,++C)
					{
						//need more space?
						if (facesRead==maxFaces)
						{
							maxFaces+=128;
							if (!tri->reserve(maxFaces))
							{
								objWarnings[NOT_ENOUGH_MEMORY]=true;
								error=true;
								break;
							}
						}

						tri->addTriangle(A->vIndex, B->vIndex, C->vIndex);
						++facesRead;

						if (hasMaterial)
							tri->addTriangleMtlIndex(currentMaterial);

						if (hasTexCoords)
							tri->addTriangleTexCoordIndexes(A->tcIndex, B->tcIndex, C->tcIndex);

						if (normalsPerFacet)
							tri->addTriangleNormalIndexes(A->nIndex, B->nIndex, C->nIndex);
					}
				}
			}
			else if (strcmp(current_token, "usemtl") == 0) // material (see MTL file)
			{
				if (materials) //otherwise we have failed to load MTL file!!!
				{
					const char* mtlName = strtok( 0, OBJ_LOADER_WHITESPACE);
					currentMaterial = (mtlName ? materials->findMaterial(mtlName) : -1);
					currentMaterialDefined = true;
				}
			}
			else if (strcmp(current_token, "mtllib") == 0) // MTL file
			{
				//we build the whole MTL filename + path
				QString mtlFilename = QString(strtok(0, "\t\n\r"));
				ccConsole::Print("[ObjFilter::Load] Material file: %s",qPrintable(mtlFilename));
				QString mtlPath = QFileInfo(filename).canonicalPath();
				//we try to load it
				if (!materials)
				{
					materials = new ccMaterialSet("materials");
					materials->link();
				}
				
				unsigned oldSize = materials->size();
				QStringList errors;
				if (ccMaterialSet::ParseMTL(mtlPath,mtlFilename,*materials,errors))
				{
					ccConsole::Print("[ObjFilter::Load] %i materials loaded",materials->size()-oldSize);
				}
				else
				{
					ccConsole::Error("[ObjFilter::Load] Failed to load material file! (should be in '%s')",qPrintable(mtlPath+'/'+mtlFilename));
					materialsLoadFailed = true;
				}

				if (!errors.empty())
				{
					for (int i=0;i<errors.size();++i)
						ccConsole::Warning("[ObjFilter::Load::MTL parser] %s",qPrintable(errors[i]));
				}
				if (materials->empty())
				{
					materials->release();
					materials=0;
					materialsLoadFailed = true;
				}
			}
			else if (strcmp(current_token, "s") == 0) // shading group
			{
				//ignored!
			}
		}

		if (error)
			break;

		//save the actual group? (if any)
		if (stop || saveCurrentGroup || (tri && createNewGroup))
		{
			//something to save?
			if (saveCurrentGroup && facesRead>0)
			{
				tri->resize(facesRead);
				totalFacesRead+=facesRead;
				if (hasMaterial || hasTexCoords)
				{
					assert(materials);
					tri->setMaterialSet(materials);
					tri->showMaterials(true);
				}
				if (hasTexCoords)
				{
					assert(texCoords);
					tri->setTexCoordinatesTable(texCoords);
				}
				if (normals && normalsPerFacet) //'per-facet' normals
				{
					normalsPerFacetGlobal = true;
					tri->setTriNormsTable(normals);
					tri->showTriNorms(true);
				}

				//we add an intermediary group to encapsulate all the sub-meshes
				meshes.push_back(tri);
			}
			else if (tri)//empty or discarded mesh ?!
			{
				if (facesRead==0)
					objWarnings[EMPTY_GROUP]=true;
				if (!saveCurrentGroup) //if we are here, it means that saving for this group has not been requested --> we discard it!
					objWarnings[DISCARED_GROUP]=true;
				delete tri;
				tri=0;
			}

			tri=0;
			maxFaces=facesRead=0;
			hasTexCoords=false;
			hasMaterial=false;
			normalsPerFacet=false;

			//job done
			saveCurrentGroup=false;
		}

		//create a new group?
		if (createNewGroup)
		{
			assert(!tri);

			tri = new ccMesh(vertices);
			tri->setName(objectName);

			//we always reserve some triangles
			maxFaces=128;
			//WARNING: don't set this value too high on Windows XP,
			//as you will really chunk the memory if you have a lot
			//of small meshes!
			if (!tri->reserve(maxFaces))
			{
				objWarnings[NOT_ENOUGH_MEMORY]=true;
				error=true;
				break;
			}

			//job done
			createNewGroup = false;
		}
	}

	fclose(fp);

	//1st check
	if (!error && pointsRead == 0)
	{
		//of course if there's no vertex, that's the end of the story ...
		ccConsole::Warning("[ObjFilter::Load] Malformed file: no vertex in file!");
		error = true;
	}

	if (!error)
	{
		ccConsole::Print("[ObjFilter::Load] %i points, %i faces",pointsRead,totalFacesRead);
		if (texCoordsRead>0 || normsRead>0)
			ccConsole::Print("[ObjFilter::Load] %i tex. coords, %i normals",texCoordsRead,normsRead);

		//do some cleaning
		if (pointsRead<maxPoints)
			vertices->resize(pointsRead);
		if (normsRead<maxNorms)
			normals->resize(normsRead);
		if (texCoordsRead<maxTexCoords)
			texCoords->resize(texCoordsRead);

		//if we have at least one mesh
		ccGenericMesh* baseMesh = 0;
		assert(!tri); //last mesh should have been save or discared properly then set to 0!
		if (!meshes.empty())
		{
			if (maxVertexIndex>=pointsRead
				|| maxTexCoordIndex>=texCoordsRead
				|| maxTriNormIndex>=normsRead)
			{
				//hum, we've got a problem here
				ccConsole::Warning("[ObjFilter::Load] Malformed file: indexes go higher than the number of elements! (v=%i/tc=%i/n=%i)",maxVertexIndex,maxTexCoordIndex,maxTriNormIndex);
				while (!meshes.empty())
				{
					delete meshes.back();
					meshes.pop_back();
				}
			}
			else
			{
				unsigned meshCount = meshes.size();
				ccConsole::Print("[ObjFilter::Load] %i mesh(es) loaded", meshCount);
				if (meshCount == 1) //don't need to keep a group for a unique mesh!
				{
					tri = meshes.front();
					baseMesh = tri;
					baseMesh->getAssociatedCloud()->setLocked(false); //DGM: no need to lock it as it is only used by one mesh!
				}
				else
				{
					ccMeshGroup* triGroup = new ccMeshGroup(vertices);
					for (unsigned i=0;i<meshCount;++i)
						triGroup->addChild(meshes[i]);
					if (normals && normalsPerFacetGlobal)
						triGroup->showTriNorms(true);
					baseMesh = triGroup;
				}

				assert(baseMesh);
				baseMesh->addChild(vertices);
				if (materials)
					baseMesh->addChild(materials,true);
				if (normals && normalsPerFacetGlobal)
					baseMesh->addChild(normals,true);
				if (texCoords)
					baseMesh->addChild(texCoords,true);

				vertices->setEnabled(false);
				vertices->setLocked(true); //by default vertices are locked (in case they are shared by mutliple sub-meshes).

				//normals: if the obj file doesn't provide any, should we compute them?
				if (!normals)
					{
					if (!materials) //yes if no material is available!
					{
						ccConsole::Print("[ObjFilter::Load] Mesh has no normal! We will compute them automatically");
						baseMesh->computeNormals();
						baseMesh->showNormals(true);
					}
					else
					{
						ccConsole::Warning("[ObjFilter::Load] Mesh has no normal! CloudCompare can try to compute them (select base entity, then \"Edit > Normals > Compute\")");
					}
				}

				container.addChild(baseMesh);
			}
		}

		if (!baseMesh)
		{
			//no (valid) mesh!
			container.addChild(vertices);
		}

		//special case: normals held by cloud!
		if (normals && !normalsPerFacetGlobal)
		{
			if (normsRead == pointsRead) //must be 'per-vertex' normals
			{
				vertices->setNormsTable(normals);
				if (baseMesh)
					baseMesh->showNormals(true);
			}
			else
			{
				ccConsole::Warning("File contains normals which seem to be neither per-vertex nor per-face!!! We had to ignore them...");
			}
		}
	}

	if (error)
	{
		if (tri)
			delete tri;
		if (vertices)
			delete vertices;
	}

	//release shared structures
	if (normals)
	{
		normals->release();
		normals=0;
	}

	if (texCoords)
	{
		texCoords->release();
		texCoords=0;
	}

	if (materials)
	{
		materials->release();
		materials=0;
	}

	progressDlg.stop();

	//potential warnings
	if (objWarnings[DISCARED_GROUP])
		ccConsole::Warning("[ObjFilter::Load] At least one group has been discared while not empty. The file might be malformed, or otherwise it's a bug! You may send us the file to help us correct it...");
	if (objWarnings[EMPTY_GROUP])
		ccConsole::Warning("[ObjFilter::Load] At least one group has been ignored because it was empty. It's generally because it is composed of unhandled components (lines, nurbs, etc.)");
	if (objWarnings[INVALID_NORMALS])
		ccConsole::Warning("[ObjFilter::Load] Some normals in file were invalid. You should re-compute them (select entity, then \"Edit > Normals > Compute\")");
	if (objWarnings[INVALID_INDEX])
		ccConsole::Warning("[ObjFilter::Load] File is malformed! Check indexes...");
	if (objWarnings[NOT_ENOUGH_MEMORY])
		ccConsole::Warning("[ObjFilter::Load] Not enough memory!");

	return (error ? (objWarnings[NOT_ENOUGH_MEMORY] ? CC_FERR_NOT_ENOUGH_MEMORY : CC_FERR_MALFORMED_FILE) : CC_FERR_NO_ERROR);
}
