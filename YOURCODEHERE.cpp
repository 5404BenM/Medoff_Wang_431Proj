#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <math.h>
#include <fcntl.h>
#include <vector>
#include <iterator>

#include "431project.h"

using namespace std;

/*
 * Enter your PSU IDs here to select the appropriate scanning order.
 */
//PSU_ID_SUM mod 24=23
#define PSU_ID_SUM (912717797+911805882)

/*
 * Some global variables to track heuristic progress.
 * 
 * Feel free to create more global variables to track progress of your
 * heuristic.
 */
 // Indexes Test order: FPU(11) -> Core(0,1) -> Cache(2~9) -> BP(12~14)
unsigned int currentlyExploringDim = 11;
bool currentDimDone = false;
bool isDSEComplete = false;

/*
 * Given a half-baked configuration containing cache properties, generate
 * latency parameters in configuration string. You will need information about
 * how different cache paramters affect access latency.
 * 
 * Returns a string similar to "1 1 1"
 */
std::string generateCacheLatencyParams(string halfBackedConfig) {

	string latencySettings;

	//
	//YOUR CODE BEGINS HERE
	//
	
	//The size of L1 instruction cache
	unsigned int il1Size = getil1size(halfBackedConfig);
	int il1Assoc = extractConfigPararm(halfBackedConfig, 6);
	//The size of L1 data cache
	unsigned int dl1Size = getdl1size(halfBackedConfig);
	int dl1Assoc = extractConfigPararm(halfBackedConfig, 4);
	//The size of unified L2 cache
	unsigned int ul2Size = getl2size(halfBackedConfig);
	int ul2Assoc = extractConfigPararm(halfBackedConfig, 9);
	
	int il1lat_idx = 0; 
	if (il1Size == 2 * 1024)
		il1lat_idx = 0;
	else if (il1Size == 4 * 1024)
		il1lat_idx = 1;
	else if (il1Size == 8 * 1024)
		il1lat_idx = 2;
	else if (il1Size == 16 * 1024)
		il1lat_idx = 3;
	else if (il1Size == 32 * 1024)
		il1lat_idx = 4;
	else if (il1Size == 64 * 1024)
		il1lat_idx = 5;
	if (il1Assoc == 1)
		il1lat_idx += 1;
	else if (il1Assoc == 2)
		il1lat_idx += 2;
	else if (il1Assoc == 3)
		il1lat_idx += 3;

	int dl1lat_idx = 0;
	if (dl1Size == 2 * 1024)
		dl1lat_idx = 0;
	else if (dl1Size == 4 * 1024)
		dl1lat_idx = 1;
	else if (dl1Size == 8 * 1024)
		dl1lat_idx = 2;
	else if (dl1Size == 16 * 1024)
		dl1lat_idx = 3;
	else if (dl1Size == 32 * 1024)
		dl1lat_idx = 4;
	else if (dl1Size == 64 * 1024)
		dl1lat_idx = 5;
	if (dl1Assoc == 1)
		dl1lat_idx += 1;
	else if (dl1Assoc == 2)
		dl1lat_idx += 2;
	else if (dl1Assoc == 3)
		dl1lat_idx += 3;

	int ul2lat_idx = 0;
	if (ul2Size == 32 * 1024)
		ul2lat_idx = 0;
	else if (ul2Size == 64 * 1024)
		ul2lat_idx = 1;
	else if (ul2Size == 128 * 1024)
		ul2lat_idx = 2;
	else if (ul2Size == 256 * 1024)
		ul2lat_idx = 3;
	else if (ul2Size == 512 * 1024)
		ul2lat_idx = 4;
	else if (ul2Size == 1024 * 1024)
		ul2lat_idx = 5;
	if (ul2Assoc == 1)
		ul2lat_idx += 1;
	else if (ul2Assoc == 2)
		ul2lat_idx += 2;
	else if (ul2Assoc == 3)
		ul2lat_idx += 3;
	else if (ul2Assoc == 4)
		ul2lat_idx += 4;
	// This is a dumb implementation.
	
	//latencySettings = "1 1 1";
	stringstream ss;
	ss << dl1lat_idx ;
	ss << " ";
	ss << il1lat_idx;
	ss << " ";
	ss << ul2lat_idx;

	latencySettings = ss.str();

	//
	//YOUR CODE ENDS HERE
	//
	//std::clog << "latency settings is :" << latencySettings << "\n";
	return latencySettings;
}

/*
 * Returns 1 if configuration is valid, else 0
 */
int validateConfiguration(std::string configuration) {

	// FIXME - YOUR CODE HERE

	if (!isNumDimConfiguration(configuration)) {
		std::cerr << "isNumDimConfiguration cannot pass.\n";
		return 0;
	}

	//The size of L1 instruction cache
	unsigned int il1Size = getil1size(configuration);
	//unsigned int il1Size = il1BlockSize * il1Assoc * il1Sets;
	//The size of L1 data cache
	unsigned int dl1Size = getdl1size(configuration);
	//The size of unified L2 cache
	unsigned int ul2Size = getl2size(configuration);

	//The block size of L1 instruction cache
	unsigned int il1BlockSize = 8 * (1 << extractConfigPararm(configuration, 2));
	//The block size of L1 data cache
	unsigned int dl1BlockSize = 8 * (1 << extractConfigPararm(configuration, 2));
	//The block size of unified L2 cache
	unsigned int ul2BlockSize = 16 << extractConfigPararm(configuration, 8);

	//std::clog << "Current Configuration is :" << configuration << "\n";
	//std::clog << "il1 size is: " << il1Size << "\n";
	//std::clog << "il1 block size is: " << il1BlockSize << "\n";
	//std::clog << "dl1 size is: " << dl1Size << "\n";
	//std::clog << "dl1 block size is: " << dl1BlockSize << "\n";
	//std::clog << "ul2 size is: " << ul2Size << "\n";
	//std::clog << "ul2 block size is: " << ul2BlockSize << "\n";

	unsigned int ifqSize =1<<extractConfigPararm(configuration, 0);
	//std::clog << "ifq size is: " << ifqSize << "\n";

	//il1BlockSize must be at least the ifqSize
	if (il1BlockSize < ifqSize) {
		//std::cerr << "il1 Block size is not at least ifq Size.\n";
		return 0;
	}
	//dl1Blocksize should equal to il1BlockSize
	if (il1BlockSize != dl1BlockSize) {
		//std::cerr << "il1 Block size is not equal to dl1 Block Size.\n";
		return 0;
	}
	//ul2BlockSize must be at least twice il1BlockSize with a maximum block size of 128B
	if (ul2BlockSize < (2 * il1BlockSize)) {
		//std::cerr << "ul2 Block size is not at least 2 times of il1 Block Size.\n";
		return 0;
	}
	if (ul2BlockSize > 128) {
		//std::cerr << "ul2 Block size is not less than 128B.\n";
		return 0;
	}
	//ul2 must be at least twice as large as il1+dl1 in order to be inclusive
	if (ul2Size < 2 * (il1Size + dl1Size)) {
		//std::cerr << "ul2 size is not at least 2 times of (il1 Size+dl1 Size).\n";
		return 0;
	}
	//il1 size and dl1 size requirement
	if (il1Size < 2 * 1024 || il1Size>64 * 1024) {
		//std::cerr << "il1 size does not satisfy the size requirement.\n";
		return 0;
	}
	if (dl1Size < 2 * 1024 || dl1Size>64 * 1024) {
		//std::cerr << "dl1 size does not satisfy the size requirement.\n";
		return 0;
	}
	//ul2 size requirement
	if (ul2Size < 32 * 1024 || ul2Size>1024 * 1024) {
		//std::cerr << "ul2 size does not satisfy the size requirement.\n";
		return 0;
	}

	/* The below is a necessary, but insufficient condition for validating a
	 configuration.*/
	//return isNumDimConfiguration(configuration);
	return 1;
}

/*
 * Given the current best known configuration, the current configuration,
 * and the globally visible map of all previously investigated configurations,
 * suggest a previously unexplored design point. You will only be allowed to
 * investigate 1000 design points in a particular run, so choose wisely.
 *
 * In the current implementation, we start from the leftmost dimension and
 * explore all possible options for this dimension and then go to the next
 * dimension until the rightmost dimension.
 */
std::string generateNextConfigurationProposal(std::string currentconfiguration,
		std::string bestEXECconfiguration, std::string bestEDPconfiguration,
		int optimizeforEXEC, int optimizeforEDP) {

	//
	// Some interesting variables in 431project.h include:
	//
	// 1. GLOB_dimensioncardinality
	// 2. GLOB_baseline
	// 3. NUM_DIMS
	// 4. NUM_DIMS_DEPENDENT
	// 5. GLOB_seen_configurations

	//std::clog << "Current Configuration is :" << currentconfiguration << "\n";
	int count[NUM_DIMS - NUM_DIMS_DEPENDENT];
	for (int i = 0; i < (NUM_DIMS - NUM_DIMS_DEPENDENT); i++)
		count[i] = 0;

	std::string nextconfiguration = currentconfiguration;
	// Continue if proposed configuration is invalid or has been seen/checked before.
	while (!validateConfiguration(nextconfiguration) ||
		GLOB_seen_configurations[nextconfiguration]) {

		// Check if DSE has been completed before and return current
		// configuration.
		if(isDSEComplete) {
			return currentconfiguration;
		}

		std::stringstream ss;

		string bestConfig;
		if (optimizeforEXEC == 1)
			bestConfig = bestEXECconfiguration;

		if (optimizeforEDP == 1)
			bestConfig = bestEDPconfiguration;

		// Fill in the dimensions already-scanned with the already-selected best
		// value.
		// Indexes Test order: FPU(11) -> Core(0,1) -> Cache(2~9) -> BP(12~14)
		for (int dim = 0; dim < currentlyExploringDim; ++dim) {
			ss << extractConfigPararm(bestConfig, dim) << " ";
		}
		/*if (currentlyExploringDim == 11) {
			for (int dim = 0; dim < 11; ++dim)
				ss << extractConfigPararm(bestConfig, dim) << " ";
				//ss << "0 ";
		}
		else if (currentlyExploringDim < 11) {
			for (int dim = 0; dim < currentlyExploringDim; ++dim)
				ss << extractConfigPararm(bestConfig, dim)<<" ";
		}
		else {
			for (int dim = 0; dim < currentlyExploringDim; ++dim) 
				ss << extractConfigPararm(bestConfig, dim) << " ";
		}*/

		// Handling for currently exploring dimension. This is a very dumb
		// implementation.
	   // Indexes Test order: FPU(11) -> Core(0,1) -> Cache(2~9) -> BP(12~14)
		if (count[currentlyExploringDim] == 0) {
			//The value in the first iteration should begin with 0 at that dimension
			ss << "0 ";
		}
		int nextValue = extractConfigPararm(nextconfiguration,
				currentlyExploringDim) + 1;
		
		//std::clog << "The Next Value is :" << nextValue << "\n";
		if (nextValue >= GLOB_dimensioncardinality[currentlyExploringDim]) {
			nextValue = GLOB_dimensioncardinality[currentlyExploringDim] - 1;
			currentDimDone = true;
		}

		if(count[currentlyExploringDim]>0){
			//For the following iterations
			ss << nextValue << " ";
		}
		count[currentlyExploringDim]++;

		if (currentlyExploringDim < 11) {
			for (int dim = (currentlyExploringDim + 1); dim < 11; ++dim)
				ss << extractConfigPararm(bestConfig, dim) << " ";
				//ss << "0 ";
		}
		// Fill in remaining independent params with 0.
		/*for (int dim = (currentlyExploringDim + 1);
			dim < (NUM_DIMS - NUM_DIMS_DEPENDENT); ++dim) {
			ss << "0 ";
		}*/
		// Indexes Test order: FPU(11) -> Core(0,1) -> Cache(2~9) -> BP(12~14)
		if (currentlyExploringDim == 11) {
			for (int dim = 12; dim < (NUM_DIMS - NUM_DIMS_DEPENDENT); ++dim)
				ss << extractConfigPararm(bestConfig, dim) << " ";
		}
		else if(currentlyExploringDim<11){
			ss<< extractConfigPararm(bestConfig, 11) << " ";

			for (int dim = 12; dim < (NUM_DIMS - NUM_DIMS_DEPENDENT); ++dim)
				ss << extractConfigPararm(bestConfig, dim) << " ";
		}
		else {
			for (int dim = (currentlyExploringDim + 1); dim < (NUM_DIMS - NUM_DIMS_DEPENDENT); ++dim)
				ss << extractConfigPararm(bestConfig, dim) << " ";
		}

		//
		// Last NUM_DIMS_DEPENDENT3 configuration parameters are not independent.
		// They depend on one or more parameters already set. Determine the
		// remaining parameters based on already decided independent ones.
		//
		string configSoFar = ss.str();

		// Populate this object using corresponding parameters from config.
		ss << generateCacheLatencyParams(configSoFar);

		// Configuration is ready now.
		nextconfiguration = ss.str();

		// Make sure we start exploring next dimension in next iteration.
		// Indexes Test order: FPU(11) -> Core(0,1) -> Cache(2~9) -> BP(12~14)
		if (currentDimDone) {
			//currentlyExploringDim++;
			if (currentlyExploringDim == 11)
				currentlyExploringDim = 0;
			else if (currentlyExploringDim == 10)
				currentlyExploringDim = 12;
			else 
				currentlyExploringDim++;
			currentDimDone = false;
		}

		// Signal that DSE is complete after this configuration.	
		if (currentlyExploringDim == (NUM_DIMS - NUM_DIMS_DEPENDENT))
			//Do not have to change, since our exploration is also done when currentlyExploringDim==15
			isDSEComplete = true;
	}
	return nextconfiguration;
}

