/******************************************************************************
 *   Copyright (C) 2008 by Marcelo Kapp - kapp@livia.etsmtl.ca				  *
 *   Modify by Jean-Francois Connolly - jfconnolly@livia.etsmtl.ca			  *
 *																			  *
 *   particle.cpp - Particle for canonical particle swarm optimizer			  *
 *   as defined in :														  *
 *																			  *
 *   J. Kennedy, Some issues and practices for particle swarms, Proc. of the  *
 *   IEEE Swarm Intelligence Symposium, 2007, pp. 162-169.					  *
 *   																		  *
 *   ----------------------------------------------------------------------   *
 *   This program is free software; you can redistribute it and/or modify it  *
 *   under the terms of the GNU General Public License as published by the    *
 *   Free Software Foundation; either version 2 of the License, or (at your   *
 *   option) any later version.                                   			  *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful, but      *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU        *
 *   General Public License for more details.                                 *
 *                                                                            *
 *   You should have received a copy of the GNU General Public License along  *
 *   with this program; if not, write to the Free Software Foundation, Inc.,  *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                *
 ******************************************************************************/

/*------------------------------- Header files -------------------------------*/
//-- Standard
#include <cstdlib>
#include <stdio.h>

//-- Project specific
#include "ensemble.h"

using namespace std;

/*============================================================================*
 *  Function name			:	particle::particle
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Constructor without arguments (default)
 *============================================================================*/
ensemble::ensemble()
{
	size       = 0;
    prediction = -1;

	//-- Arrays in the research space
	vote         = (int*    )NULL;
	members      = (int*    )NULL;
	tieBreakerCl = (float*  )NULL;
	tieBreakerSz = (float*  )NULL;
	nn           = (artmap**)NULL;
	pFitness     = (result**)NULL;
}
/*============================================================================*
 *  Function name			:	ensemble::ensemble
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Initialization
 *----------------------------------------------------------------------------*
 *  Arguments				:   int nDim 	- Number of dimensions
 *  							int nObj	- Number of objective
 *  							int xMin	- Minimal bound
 *  							int nObj	- Maximal bound
 *============================================================================*/
void ensemble::initialization(configuration *cfg)
{
	//-- General
	size        = 0;
	int sizeMax = cfg->sizeMemetics * (cfg->sizeF2max / cfg->widthMemetics);
	if(cfg->sizeSwarm > sizeMax)   { sizeMax = cfg->sizeSwarm; }
	prediction  = -1;

	//-- Potentially, and ensemble the size of the swarm
	members = new int[sizeMax];
	for(int e=0; e<sizeMax; e++)   { members[e] = -1; }
	
	//-- Test data base
	char nameFile[100];
	sprintf(nameFile, "%s/%s/test.db", cfg->pathDb, cfg->nameDb);
	dbInit.initialiHeader(nameFile);

	if(dbInit.nPatterns == 0)
	{ printf("Error - no data base loaded for the ensemble\n", nameFile); }

	//-- Ensemble of neural networks and their fitness
	nn       = new artmap*[sizeMax];
	pFitness = new result*[sizeMax];

	for(int e=0; e<sizeMax; e++)
	{	nn[e] = (artmap*)NULL;   pFitness[e] = (result*)NULL;   }

	//-- Vote counter for each classes
	vote         = new int  [ dbInit.nClasses ];
	tieBreakerCl = new float[ dbInit.nClasses ];
	tieBreakerSz = new float[ dbInit.nClasses ];

	//-- The ensemble's performance
	performances.initialization( dbInit.nClasses, dbInit.nPatterns,
								 cfg->sizeSwarm );
}
/*============================================================================*
 *  Function name			:	ensemble::ensemble
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Initialization
 *----------------------------------------------------------------------------*
 *  Arguments				:   int nDim 	- Number of dimensions
 *  							int nObj	- Number of objective
 *  							int xMin	- Minimal bound
 *  							int nObj	- Maximal bound
 *============================================================================*/
void ensemble::reinitialization()
{
// 	printf("Reinit, size: %d\n", size);
	for(int e=0; e<size; e++)
	{	members [e] = -1;
		nn      [e] = (artmap*)NULL;
		pFitness[e] = (result*)NULL;
	}

	performances.reinit();
	size = 0;
}

/*============================================================================*
 *  Function name			:	ensemble::~ensemble
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Destructor
 *============================================================================*/
ensemble::~ensemble()
{
	size       = 0;
	prediction = -1;

	delete [] vote;
	delete [] members;
	delete [] tieBreakerCl;
	delete [] tieBreakerSz;
	delete [] nn;
	delete [] pFitness;
}
/*============================================================================*
 *  Function name			:	ensemble::initialization
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Random initialization of a ensemble (can also
 *  							be used for re-initialization).
 *============================================================================*/
void ensemble::test()
{
	int	nClasses  = dbTest->nClasses;
		
	//-- Reinitialize the result
	performances.reinit();

	/*-------------- For each active classes, test all patterns --------------*/
	//-- Find the number of test patterns
	int nPatterns = 0, addrResult = -1;
	for(int k=0; k<nClasses; k++)
	{ if(dbTest->activeClasses[k])  { nPatterns += dbTest->sizeClasses[k]; } }

	//-- Test the active classes
	for(int k=0; k<nClasses; k++)
	{
		if(dbTest->activeClasses[k])
		{
			//-- Number of patterns for the current class & address
			int nPatternsClass = dbTest->sizeClasses[k];
			int addrClass      = dbTest->addrClasses[k];

			for(int p=0; p<nPatternsClass; p++)
			{
				//-- Addresses
				addrResult +=1;
				int addrDb  = addrClass + p;

				//-- Initialize the vote
 				for(int k2=0; k2<nClasses; k2++)
 				{ vote[k2] = 0;  tieBreakerCl[k2] = 0;  tieBreakerSz[k2] = 0; }

				//-- Get the vote of each member and update the tie breakers
				for(int e=0; e<size; e++)
				{   
					prediction       = nn[e]->prediction(dbTest, addrDb);
					vote[prediction] = vote[prediction] + 1;

					/*-------- Updates the ensemble performances - 1/3 -------*/
					//-- Predictions
					performances.predictions[e*nPatterns + addrResult] =
							                                         prediction;
					//-- Tie breakers
					tieBreakerCl[prediction] = tieBreakerCl[prediction] +
												 pFitness[e]->nClassOk;
					tieBreakerSz[prediction] = tieBreakerSz[prediction] +
												 pFitness[e]->sizeNn;
				}
				
				//-- Finds the prediction ...
				prediction = 0;
				float bestClRate = 0, bestSize = 2000;
				for(int k2=0; k2<nClasses; k2++)
				{	//-- ... here ...
					int winner = 0;
					if     (vote[k2] >  vote[prediction])		{ winner = 1; }
					else if(vote[k2] == vote[prediction])
					{	//-- Tie breaker 1 - Average classification rate
						if     (tieBreakerCl[k2] > bestClRate)  { winner = 1; }
						//-- Tie breaker 2 - Total network size
						else if(tieBreakerCl[k2] == bestClRate &&
								tieBreakerSz[k2] <  bestSize)	{ winner = 1; }
					}

					//-- ... and here (update the winning class)
					if(winner)
					{	prediction = k2;
						bestClRate = tieBreakerCl[k2];
						bestSize   = tieBreakerSz[k2];
					}
				}

				/*---------- Updates the ensemble performances - 2/3 ---------*/
				//-- Classification table update
				performances.predisemble[addrResult] = prediction;
				performances.trueClasses[addrResult] = dbTest->labels[addrDb];

				if(prediction == dbTest->labels[addrDb])
				{   performances.nClassOk = performances.nClassOk +1;   }
				else
				{   performances.nClassWg = performances.nClassWg +1;   }
			}//- end : for nPatternsClass
		}//- end : if activeClasses[k]
	}//- end : for nClasses

	/*---------------- Updates the ensemble performances - 3/3 ---------------*/
	//-- Average size of the ensemble and number of patterns
	performances.sizeEns          = size;
	performances.nPatternsTest    = nPatterns;
	//- performances.nPatternsLearned = nn[0]->nPatternsLearned; see main!
	performances.clsRate          = performances.nClassOk /
								(performances.nClassOk + performances.nClassWg);

	//-- Members, and convergence time and class sizes of each network
	int sizeF2total = 0;
	for(int e=0; e<size; e++)
	{	performances.members    [e] = members[e];
		performances.membersTime[e] = nn[e]->nEpochs;

		for(int k=0; k<nClasses; k++)
		{ performances.sizeClasses[e*nClasses + k]  = nn[e]->sizeClasses[k];
		  sizeF2total 							   += nn[e]->sizeClasses[k]; }
	}

	float sizeF2av = (float)sizeF2total / (float)size;
	performances.sizeNn  = sizeF2total;
	performances.normCpn = (performances.nPatternsLearned / sizeF2av -1) / 
						   (performances.nPatternsLearned / nClasses -1);
}
/*============================================================================*
 *  Function name			:	ensemble::initialization
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Random initialization of a ensemble (can also
 *  							be used for re-initialization).
 *============================================================================*/
void ensemble::save(char *nameFile, int currentRep, int currentBlock)
{
	for(int e=0; e<size; e++)
	{   nn[e]->save(nameFile, currentRep, currentBlock);   }
}
/*============================================================================*
 *  Function name			:	ensemble::assignDbTest
 *  Originally written by	:	Jean-Francois Connolly
 *  Description				:	Assign the test data base (i.e. a setter).
 *============================================================================*/
void ensemble::assignDbTest(dbase *db)   { dbTest = db; }
