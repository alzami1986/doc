/******************************************************************************
 *  Copyright(C) 2009 by Jean-Francois Connolly - jfconnolly@livia.etsmtl.ca  *
 *																			  *
 *  fitness.h - Fitness evaluation object. Fitness is evaluated as the        *
 *  fitnesss after training a classifier and it is possible to evaluate   *
 *  several objective at each iteration.                                      *
 *                                                                            *
 *  ----------------------------------------------------------------------    *
 *  This program is free software; you can redistribute it and/or modify it   *
 *  under the terms of the GNU General Public License as published by the     *
 *  Free Software Foundation; either version 2 of the License, or (at your    *
 *  option) any later version.                                   			  *
 *                                                                            *
 *  This program is distributed in the hope that it will be useful, but       *
 *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 *  General Public License for more details.                                  *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License along   *
 *  with this program; if not, write to the Free Software Foundation, Inc.,   *
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                 *
 ******************************************************************************/

/*------------------------------- Header files -------------------------------*/
//-- Standard
#include <stdio.h>
#include <string.h>

//-- Project specific
#include "objFtn_fam.h"

using namespace std;
/*============================================================================*
 *  Function name			:	objFtn_fam::objFtn_fam
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Constructor for fitness evaluation (default)
 *============================================================================*/
objFtn_fam::objFtn_fam()
{
	//-- Constant
	nDimensions    = 0;
	nObjectives    = 0;
	nEstimationRep = 0;
	nBlocks        = 0;
	nReplications  = 0;

	//-- Every thing is NULL and the networks  before and after fitness
	//   estimation are not initialized
	solution     = (float*)NULL;
}
/*============================================================================*
 *  Function name			:	objFtn_fam::initEstimation
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Initialization for fitness estimation for
 *  							one replication
 *----------------------------------------------------------------------------*
 *  Arguments				:   configuration cfg  - Configuration
 *								int           r	   - Current replication
 *============================================================================*/
void objFtn_fam::initEstimation(configuration *cfg, int r)
{
	//-- Constant
	nDimensions    = cfg->nDimensions;
	nObjectives    = cfg->nObjectives;
	nEstimationRep = cfg->nEstimationRep;
	nBlocks        = cfg->nBlocks;
	nReplications  = cfg->nReplications;
	typeSpace	   = fitEstimation;

	//-- Number of classes and features
	char nameFile[100];
	dbase dbInit;
	sprintf(nameFile, "%s/%s/test.db", cfg->pathDb, cfg->nameDb);
	dbInit.initialiHeader(nameFile);
		
	int nClasses  = dbInit.nClasses;
	int nFeatures = dbInit.nFeatures;
	int nEpoMax   = cfg->nEpochMax;
	int sizeNN    = cfg->sizeF2max;

	//-- Simplified version of the results used for validation
	//   and fitness estimation -> does NOT need initialization

	//-- Neural networks - for all objectives
	nnBfe.initialization(nClasses, nFeatures, sizeNN, nEpoMax);
	nnAfr.initialization(nClasses, nFeatures, sizeNN, nEpoMax);

	nnVal.initialization(nClasses, nFeatures, sizeNN, nEpoMax);
	nnFit.initialization(nClasses, nFeatures, sizeNN, nEpoMax);

	//-- Input/output
	solution         = new float[nDimensions];
	fitness.nClasses = nClasses;
}
/*============================================================================*
 *  Function name			:	objFtn_fam::~objFtn_fam
 *  Originally written by	:	Marcelo Kapp
 *  Modified by				:	Jean-Francois Connolly
 *  Description				:	Destructor
 *============================================================================*/
objFtn_fam::~objFtn_fam()
{	if(solution)		{ delete [] solution; }   }
/*============================================================================*
 *  Function name			:	objFtn_fam::loadDb
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Load data bases
 *----------------------------------------------------------------------------*
 *  Arguments				:   configuration cfg  - Configuration
 *								int           time - Current time
 *								int           r	   - Current replication
 *============================================================================*/
void objFtn_fam::loadDb(configuration *cfg, int time, int r)
{
	char db[5], Ext[5], nameFile[100];
  if( cfg->incremental )   { sprintf(db,"%s","D");  sprintf(Ext,"%s","Ext"); }
  else                     { sprintf(db,"%s","B");  sprintf(Ext,"%s",""   ); }

	//o------------------------------ Loading DB ------------------------------o//
	//-- Training data sets
	sprintf(nameFile, "%s/%s/%s%dBlocks%s/%s%d^t%d.db", cfg->pathDb,
			cfg->nameDb, cfg->nameSc, nBlocks, Ext, db, time+1, r+1);
	dbTrn.initialization(nameFile);

	//-- Validation data sets
	sprintf(nameFile, "%s/%s/%s%dBlocks%s/%s%d^v%d.db", cfg->pathDb,
			cfg->nameDb, cfg->nameSc, nBlocks, Ext, db, time+1, r+1);
	dbVal.initialization(nameFile);

	//-- Particles fitness evaluation data sets
	sprintf(nameFile, "%s/%s/%s%dBlocks%s/%s%d^f%d.db", cfg->pathDb,
			cfg->nameDb, cfg->nameSc, nBlocks, Ext, db, time+1, r+1);
	dbFit.initialization(nameFile);
	
	
	//o----------------- Number of training patterns so far -----------------o//
	//-- The total number of training patterns (i.e., FAM maximal size)
	//   inc -> we accumulate, batch -> we do not!
	dbase dbInit;
	nAccPatterns = 0;
		
	for(int t=0; t<=time; t++)
	{	sprintf(nameFile, "%s/%s/%s%dBlocks%s/%s%d^t%d.db", cfg->pathDb,
				cfg->nameDb, cfg->nameSc, nBlocks, Ext, db, t+1, r+1);
		dbInit.initialiHeader(nameFile);
		
		if(cfg->incremental)   { nAccPatterns += dbInit.nPatterns; }
		else				   { nAccPatterns  = dbInit.nPatterns; }
	}
}
/*============================================================================*
 *  Function name			:	objFtn_fam::deleteDb
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Delete data bases
 *============================================================================*/
void objFtn_fam::deleteDb()
{
	dbTrn.~dbase();
	dbVal.~dbase();
	dbFit.~dbase();
}
/*============================================================================*
 *  Function name			:	objFtn_fam::estimation
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Estimation of a value on the objective
 * 								function (i.e. estimation of fitness)
 ============================================================================*/
void objFtn_fam::estimation()
{
	float bestClsOk = 0, bestsizeNn = 0;
	/*--------------- Estimation - best replication out of 5 ----------------*/
	for(int rInt=0; rInt<nEstimationRep; rInt++)
	{
		//-- ARTMAP used for training and testing initialization
		nnFit.copy(&nnBfe);

		//-- Setting hyperparameter's values to particule's position
		nnFit.setHp(solution[0],solution[1],solution[2],solution[3]);

		//-- Training
		trainV(&nnFit, &nnVal, &dbTrn, &dbVal, &resultFt, &resultVl);

		//-- Test
		test(&nnFit, &dbFit, &resultFt);

		//-- We keep the best replication (result & network)
		//   In cases of equality, the smallest network is chosen.
		if(  resultFt.nClassOk >  bestClsOk ||
		    (resultFt.nClassOk == bestClsOk && resultFt.sizeNn < bestsizeNn) )
		{	nnAfr.copy(&nnFit);
			fitness.copy(&resultFt);
			bestClsOk  = resultFt.nClassOk;
			bestsizeNn = resultFt.sizeNn;
		}
	}
	//-- Final adjustements
	if(fitness.normCpn < 0)   { fitness.normCpn = 0; }
	nnAfr.nPatternsLearned = nAccPatterns;
}
/*============================================================================*
 * Function name			:	trainV
 * Originally written by	:	Jean-Francois Connolly
 * Modified by				:	---
 * Description				:	Learning with validation
 *----------------------------------------------------------------------------*
 * Argument		: artmap *bestNN       - ARTMAP neural network
 * Argument		: artmap *currentNN    - ARTMAP neural network
 * Argument		: result *bestResult   - Best result so far
 * Argument		: result *valResult    - Validation result
 * Argument		: dbase* dbTrain       - Train data base
 * Argument		: dbase* dbValid       - Validation data base
 * Argument		: int 	 classifier    - Classifier type
 *============================================================================*/
void objFtn_fam::trainV(artmap *bestNN, 	artmap *currentNN,
						dbase  *dbTrain, 	dbase  *dbValid,
						result *bestResult, result *valResult)
{
	int	bestEpoch  = 0,
		n          = 0,
		nEpochMax  = bestNN->nEpochMax,
		nThreshold = 2;				//-- Number of additional training epochs
									//   to avoid local minimums

	//o--------- Network, result order presentation reinitialization --------o//
	currentNN->copy(bestNN);
	bestResult->reinit();

/*	//o------------------------- One epoch learning -------------------------o//
	train(currentNN, dbTrain);
	bestNN->copy(currentNN);
	bestNN->nEpochs = 1;
*/
	//o---------------------- Learning with validation ----------------------o//
	while(n < nEpochMax && n-bestEpoch < nThreshold)
	{	n++;

		//-- Training & validation
		train(currentNN, dbTrain);
		test (currentNN, dbValid, valResult);

		//-- If classification rate s better,
		//   the network is kept and training continues.
		if( valResult->nClassOk > bestResult->nClassOk )
		{
			bestResult->nClassOk = valResult->nClassOk;
			bestNN->copy(currentNN);
			bestEpoch = n;
		}
	}
	//-- The best number of training epochs is kept
	bestNN->nEpochs = bestEpoch;
}
/*============================================================================*
 * Function name			: objFtn_fam::train
 * Originally written by	: Jean-Francois Connolly
 * Description				: Network training (1 epoch)
 *----------------------------------------------------------------------------*
 * Argument					: artmap   	*myNN		- ARTMAP neural network
 * Argument					: result 	*myResult	- Result
 * Argument					: dbase 	*dbTrain	- Test data base
 *============================================================================*/
void objFtn_fam::train(artmap *nn, dbase *dbTrain)
{
	//-- Train
	nn->train(dbTrain);
}
/*============================================================================*
 * Function name			: objFtn_fam::test
 * Originally written by	: Jean-Francois Connolly
 * Description				: Test network and updates a simplified result
 * 							  (for fitness estimation only)
 *----------------------------------------------------------------------------*
 * Argument					: artmap   	*myNN		- ARTMAP neural network
 * Argument					: result 	*myResult   - Result
 * Argument					: dbase 	*dbTest		- Test data base
 *============================================================================*/
void objFtn_fam::test(artmap *nn, dbase *dbTest, result *myResult)
{
	//-- Result initialization
	myResult->reinit();

	//-- Prediction loop
	int nPatterns = dbTest->nPatterns;

	//o----- Prediction and update result - classification table update -----o//
	for(int p=0; p<nPatterns; p++)
	{
		int prediction = nn->prediction(dbTest, p);
		if(prediction == dbTest->labels[p])   { myResult->nClassOk += 1; }
		else                      			  { myResult->nClassWg += 1; }
	}
	
	myResult->clsRate  = myResult->nClassOk /
	                     ( myResult->nClassOk + myResult->nClassWg );

	//o---- Update result - class sizes, compression, & convergence time ----o//
	myResult->sizeNn   = nn->sizeF2;
	myResult->nClasses = nn->nClasses;
	myResult->nPatternsLearned = nAccPatterns; 
	myResult->normCpn = ((float)nAccPatterns / myResult->sizeNn         -1)/
	                    ((float)nAccPatterns / (float)myResult->nClasses-1);

//	printf("/-- %1.2f, %1.2f -> %1.2f -- %1.2f, %d, %d -> %1.2f\n",
//	       myResult->nClassOk, myResult->nClassWg, myResult->clsRate,
//		   myResult->sizeNn,   nAccPatterns[time], myResult->nClasses, 
//		   myResult->normCpn);

	myResult->convTime = nn->nEpochs;
}
