/*
	This file is part of SHMUP.

    SHMUP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SHMUP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/    
/*
 *  camera.c
 *  dEngine
 *
 *  Created by fabien sanglard on 15/08/09.
 *  Copyright 2009 Memset software Inc. All rights reserved.
 *
 */

#include "camera.h"
#include "renderer.h"
#include "timer.h"
#include "filesystem.h"
#include "lexer.h"
#include "world.h"
#include "collisions.h"
#include "preproc.h"
#include "vis.h"

#ifdef ANDROID
#include "../android/android_utils.h"
#endif

camera_t camera;

matrix3x3_t orientationMatrix;

int cameraVisMemSize;

void CAM_InterpolateFrames(camera_frame_t* currentFrame, camera_frame_t* nextFrame, float interpolationFactor, vec3_t position, quat4_t orientation)
{
	
	//Interpolate position
	vectorLinearInterpolate(currentFrame->position,nextFrame->position,interpolationFactor,position);
	
	
	
	//Interpolate quaternion
	Quat_slerp(currentFrame->orientation, nextFrame->orientation, interpolationFactor, orientation);
	
	
}

void CAM_FreeCameraFrame(camera_frame_t* toDelete)
{
	int i;
	
	if (toDelete->visUpdate.isKey)
	{
		for (i=0; i<  toDelete->visUpdate.numVisSets; i++) 
			free(toDelete->visUpdate.visSets[i].indices);
	}
	else 
	{
		for (i=0; i<  toDelete->visUpdate.numVisSets; i++) 
		{
			free(toDelete->visUpdate.visSets[i].facesToAdd);
			free(toDelete->visUpdate.visSets[i].facesToRemove);
		}
	}

	free(toDelete->visUpdate.visSets);
	free(toDelete);
}

void CAM_ClearAllRemainingCameraVS(void)
{
	camera_frame_t* toDelete;
	
	if (camera.currentFrame == NULL)
		return;
	
	while (camera.currentFrame->next != NULL && camera.currentFrame->next->time <= simulationTime)
	{
		toDelete =  camera.currentFrame;
		camera.currentFrame = camera.currentFrame->next;
		CAM_FreeCameraFrame(toDelete);
	}
}

void CAM_Update(void)
{
	float			interpolationFactor;
	quat4_t			interpolatedQuaterion;
	matrix3x3_t		interpolatedOrientationMatrix;
	camera_frame_t* nextFrame = 0;
	camera_frame_t* toDelete = 0;
	
	if (!camera.playing)
		return;
	
	
	//printf("CAM_Update\n");
	//printf("camera.currentFrame->next=%d\n",camera.currentFrame->next);
	//printf("camera.currentFrame->next->time=%d\n",camera.currentFrame->next->time);
	//printf("simulationTime=%d\n",simulationTime);

	while (camera.currentFrame->next != NULL && camera.currentFrame->next->time <= simulationTime)
	{
		//Update vis_set if not already done, take into account key frame_update
		//printf("Jumping into vis_update().\n");
		VIS_Update();
		
		toDelete = camera.currentFrame;
		camera.currentFrame = camera.currentFrame->next;
		CAM_FreeCameraFrame(toDelete);
	}	
		
	//printf("frame t=%d.\n",camera.currentFrame->time);
	
	if (camera.currentFrame->next == 0)
		return;
		
	nextFrame = camera.currentFrame->next;
		
	
	interpolationFactor = (simulationTime - camera.currentFrame->time) * 1.0 / (nextFrame->time - camera.currentFrame->time);
	//printf("interpo = %.2f\n",interpolationFactor);
		
	CAM_InterpolateFrames(camera.currentFrame,nextFrame,interpolationFactor, camera.position, interpolatedQuaterion);
		
	//Transforme quat to matrix and set it as orientation
	Quat_ConvertToMat3x3(interpolatedOrientationMatrix, interpolatedQuaterion);
		
	
	//printf("Camera pos: [%.2f,%.2f,%.2f].\n",camera.position[0],camera.position[1],camera.position[2]);
	//printf("Camera orientation matrix:\n");
	//matrix_print3x3(interpolatedOrientationMatrix);
	//printf("Camera orientation quaternion: [%.5f,%.5f,%.5f,%.5f]\n",interpolatedQuaterion[0],interpolatedQuaterion[1],interpolatedQuaterion[2],interpolatedQuaterion[3]);

	
	camera.right[0] = interpolatedOrientationMatrix[0];
	camera.right[1] = interpolatedOrientationMatrix[1];
	camera.right[2] = interpolatedOrientationMatrix[2];
	
	camera.up[0] = interpolatedOrientationMatrix[3];
	camera.up[1] = interpolatedOrientationMatrix[4];
	camera.up[2] = interpolatedOrientationMatrix[5];
	
	
	camera.forward[0] = -interpolatedOrientationMatrix[6];
	camera.forward[1] = -interpolatedOrientationMatrix[7];
	camera.forward[2] = -interpolatedOrientationMatrix[8];	

}



void CAM_StartPlaying()
{
	camera.playing = 1;
}




camera_frame_t* CAM_ReadFrameCP2Binary(FILE* fileHandle)
{
	camera_frame_t* frame;
	world_vis_set_update_t* worldVisSet;
	entity_visset_t* entityVisSet;
	int j;
	
	frame = calloc(1, sizeof(camera_frame_t));
	
	cameraVisMemSize += sizeof(camera_frame_t);
	
	fread(&frame->time, sizeof(frame->time), 1, fileHandle);
	
	//printf("time = %d:",frame->time);
	
	fread(&frame->position[X], sizeof(float), 1, fileHandle);
	fread(&frame->position[Y], sizeof(float), 1, fileHandle);
	fread(&frame->position[Z], sizeof(float), 1, fileHandle);
	
	fread(&frame->orientation[X], sizeof(float), 1, fileHandle);
	fread(&frame->orientation[Y], sizeof(float), 1, fileHandle);
	fread(&frame->orientation[Z], sizeof(float), 1, fileHandle);
	fread(&frame->orientation[W], sizeof(float), 1, fileHandle);
	
	worldVisSet = &frame->visUpdate ;
	
	fread(&worldVisSet->isKey, sizeof(uchar), 1, fileHandle);
	
	fread(&worldVisSet->numVisSets, sizeof(ushort), 1, fileHandle);
	
	worldVisSet->visSets = calloc(worldVisSet->numVisSets, sizeof(entity_visset_t));
	cameraVisMemSize += worldVisSet->numVisSets * sizeof(entity_visset_t) ;
	
//	printf("	Reading visSet: isKey=%2d.\n",worldVisSet->isKey);
	
	for(j=0 ; j < worldVisSet->numVisSets ; j++)
	{
		entityVisSet = &worldVisSet->visSets[j];
		
		fread(&entityVisSet->entityId, sizeof(ushort), 1, fileHandle);
		
	
		
		if (worldVisSet->isKey)
		{
		
			fread(&entityVisSet->numIndices, sizeof(ushort), 1, fileHandle);
	
			entityVisSet->indices = calloc(entityVisSet->numIndices, sizeof(ushort));
			cameraVisMemSize += sizeof(entityVisSet->numIndices * sizeof(ushort));
		
			fread(entityVisSet->indices, sizeof(ushort), entityVisSet->numIndices, fileHandle);
	
			
		}
		else 
		{
			
			
			
			fread(&entityVisSet->numFacesToAdd, sizeof(ushort), 1, fileHandle);
			entityVisSet->facesToAdd = calloc(entityVisSet->numFacesToAdd, sizeof(ushort));
			cameraVisMemSize += sizeof(entityVisSet->numFacesToAdd * sizeof(ushort));
			fread(entityVisSet->facesToAdd, sizeof(ushort), entityVisSet->numFacesToAdd, fileHandle);
			
			fread(&entityVisSet->numFacesToRemove, sizeof(ushort), 1, fileHandle);
			entityVisSet->facesToRemove = calloc(entityVisSet->numFacesToRemove, sizeof(ushort));
			cameraVisMemSize += sizeof(entityVisSet->numFacesToRemove * sizeof(ushort));
			fread(entityVisSet->facesToRemove, sizeof(ushort), entityVisSet->numFacesToRemove, fileHandle);

			
			
		}

	}
	
	
	return frame ;
}

camera_frame_t* CAM_ReadFileCP2Binary(char* f,char prependGameDir)
{
	char* magicNumber = "CP2B" ;
	char  magicCheck[5];
	FILE* fileHandle;
	int num_frames= 0 ;
	int i;
	char filename[256];
	
	camera_frame_t* frame;
	camera_frame_t firstFrame;
	
	memset(filename, 0, sizeof(filename));
	if (prependGameDir)
	{
		strcat(filename, FS_Gamedir());
		strcat(filename, "/");
	}
	
	strcat(filename, f);
	
	#ifndef ANDROID
	fileHandle = fopen(filename, "rb");
	#else
	fileHandle = fopen(getAndroidFilename(filename), "rb");
	#endif

	if (!fileHandle)
	{
		printf("[CAM_ReadFileCP2Binary] Could not load binary cp2 (%s).\n",filename);
		return 0;
	}
	
	fread(magicCheck, 4, sizeof(char), fileHandle);
	magicCheck[4] = '\0';
	
	if (strcmp(magicNumber, magicCheck))
	{
		printf("[CAM_ReadFileCP2Binary] Found binary cp2 (%s) but magic number check failed.\n",filename);
		return 0;
	}
	
	fread(&num_frames, sizeof(num_frames), 1, fileHandle);
	
	printf("[CAM_ReadFileCP2Binary] Found %d frames.\n",num_frames);
	
	frame = &firstFrame ;
	
	for(i=0 ; i < num_frames ; i++)
	{
		//printf("Reading binary frame %d/%d: t=",i+1,num_frames);
		frame->next = CAM_ReadFrameCP2Binary(fileHandle);
		frame= frame->next;
	}
	
	fclose(fileHandle);
	
	return firstFrame.next;
}


void CAM_ExpandCameraWayPoints(camera_frame_t* startFrame,camera_frame_t* endFrame)
{
	camera_frame_t*	newFrame;
	camera_frame_t*	currentFrame;
	int						timeDifference;
	float					interpolationFactor;
	
	int time;
	int extraAccuracyTime;
	
	timeDifference = endFrame->time - startFrame->time;
	
	currentFrame = startFrame;
	
	time = startFrame->time;
	extraAccuracyTime=0;
	
	while (currentFrame->time  < endFrame->time) 
	{
		
		
		newFrame = (camera_frame_t*)calloc(1, sizeof(camera_frame_t));
		
		newFrame->time += currentFrame->time + 16+ (int)extraAccuracyTime ;
		
		
		
		newFrame->next = endFrame;
		currentFrame->next = newFrame;
		
		
		
		//Interpolate position and oritentation for the given time.
		
		interpolationFactor = (newFrame->time - startFrame->time) / (float)timeDifference;
		CAM_InterpolateFrames(startFrame,endFrame,interpolationFactor,newFrame->position,newFrame->orientation);
		
		
		
		currentFrame = newFrame;
		extraAccuracyTime+=0.6666667f;
	}
}

void CAM_LoadPath(void)
{	
	//Check extension
	char* extension; 
	int traceID;
	char binPath[256]; 
	//char logPath[256]; 
	
	cameraVisMemSize = 0;
	
	if (camera.pathFilename[0] == '\0')
	{
		printf("[CAM_LoadPath] No camera path loaded. Aborting.");
		exit(0);
	}
	
	extension = FS_GetExtensionAddress(camera.pathFilename);

	
	camera.path = NULL;
	
	if (!strcmp(extension, "cp")) 
	{
		//Need to transform from CP to CP2B
		traceID = rand();
		memset(binPath, 0, 256);
		sprintf(binPath, "/Users/fabiensanglard/tmp/%i",traceID);
		strcat(binPath,"intro.cp.cp2b");
		
		/*
		memset(logPath, 0, 256);
		sprintf(logPath, "/Users/fabiensanglard/tmp/%i",traceID);
		strcat(logPath,".log.txt");
		*/
		
		PREPROC_ConvertCp1Tocp2b(camera.pathFilename,binPath,0);
		camera.path =          CAM_ReadFileCP2Binary(binPath,0);
		
		//camera.path = CAM_ReadFileCP(camera.pathFilename);
	}
	else 
	{
		camera.path = CAM_ReadFileCP2Binary(camera.pathFilename,1);
	}

	
	if (camera.path == NULL)
	{
		printf("[CAM_LoadPath] Could not load camera path properly. Aborting.\n");
		exit(0);
	}
	
	
	camera.currentFrame = camera.path;
	simulationTime = camera.currentFrame->time;
	
	printf("[CAM_LoadPath] found and loaded %s.\n",camera.pathFilename);
//	printf("Camera path is taking %d kb in main memory.\n",cameraVisMemSize/1024);
	
}




