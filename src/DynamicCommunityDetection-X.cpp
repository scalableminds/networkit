//============================================================================
// Name        : DynamicCommunityDetection.cpp
// Author      : Christian Staudt (christian.staudt@kit.edu),
//				 Henning Meyerhenke (henning.meyerhenke@kit.edu)
// Version     :
// Copyright   : � 2013, Christian Staudt, Henning Meyerhenke
// Description : Framework for engineering dynamic community detection algorithms
//============================================================================

// includes


// includes
#include <iostream>
#include <fstream>
#include <utility>
#include <cfenv>	// floating point exceptions
#include <stdexcept>
#include <vector>


// GoogleTest
#ifndef NOGTEST
#include <gtest/gtest.h>
#endif

// OpenMP
#include <omp.h>


// EnsembleClustering
#include "Globals.h"
#include "ext/optionparser.h"
#include "auxiliary/Log.h"
#include "auxiliary/Timer.h"
#include "auxiliary/Functions.h"
#include "auxiliary/StringTools.h"
#include "graph/Graph.h"
#include "dcd/PseudoDynamic.h"
#include "io/METISGraphReader.h"
#include "generators/DynamicBarabasiAlbertGenerator.h"
#include "generators/DynamicDGSParser.h"
#include "dcd/DynamicCommunityDetector.h"
#include "dcd/TDynamicLabelPropagation.h"
#include "dcd/DynamicLabelPropagation.h"
#include "dcd/DynCDSetup.h"
#include "community/PLP.h"
#include "community/PLM.h"
#include "community/EPP.h"
#include "dcd/DynamicEnsemble.h"
#include "overlap/Overlapper.h"
#include "overlap/HashingOverlapper.h"

using namespace NetworKit;






/**
 *  Set the number of threads available to the program.
 */
void setNumberOfThreads(int nThreads) {
#ifdef _OPENMP
		omp_set_num_threads(nThreads);
#else
		WARN("Thread option ignored since OpenMP is deactivated.");
#endif
}



// *** Option Parser Configuration ***//

class Arg: public OptionParser::Arg {

static OptionParser::ArgStatus Required(const OptionParser::Option& option, bool msg)
{
  if (option.arg != 0)
    return OptionParser::ARG_OK;

  if (msg) {
	  std::cout << "Option '" << option << "' requires an argument" << std::endl;
  }
  return OptionParser::ARG_ILLEGAL;
}

};

// TODO: clean up obsolete parameters
enum  optionIndex { UNKNOWN, HELP, LOGLEVEL, THREADS, TESTS, SOURCE, DETECTORS, TMAX, DELTAT, STATIC, CHECK_COMMUNITIES, CHECK_CONTINUITY, RUNS, SAVE_GRAPH, PROGRESS, SUMMARY, UPDATE_THRESHOLD, SAVE_CLUSTERINGS};
const OptionParser::Descriptor usage[] =
{
 {UNKNOWN, 0,"" , ""    ,OptionParser::Arg::None, "USAGE: EnsembleClustering [options]\n\n"
                                            "Options:" },
 {HELP,    0,"h" , "help",OptionParser::Arg::None, "  --help  \t Print usage and exit." },
 {LOGLEVEL,    0, "" , "loglevel", OptionParser::Arg::Required, "  --loglevel=<LEVEL>  \t set the log level" },
 {THREADS,    0, "" , "threads", OptionParser::Arg::Required, "  --threads=<NUM>  \t set the maximum number of threads" },
 {TESTS, 0, "t", "tests", OptionParser::Arg::None, "  --tests \t Run unit tests"},
 {SOURCE, 0, "", "source", OptionParser::Arg::Required, "  --source=<NAME>:<PARAMS> \t select source of dynamic graph"},
 {DETECTORS, 0, "", "detectors", OptionParser::Arg::Required, "  --detectors=<NAME>:<PARAMS> \t select dynamic community detection algorithms"},
 {TMAX, 0, "", "tMax", OptionParser::Arg::Required, "  --tMax=<INT> \t maximum number of generator time steps"},
 {DELTAT, 0, "", "deltaT", OptionParser::Arg::Required, "  --deltaT=<INT> \t run the detectors each deltaT time steps"},
 {STATIC, 0, "", "static", OptionParser::Arg::Required, "  --static=<NAME> \t select static algorithm"},
 {CHECK_COMMUNITIES, 0, "", "checkCommunities", OptionParser::Arg::None, "  --checkCommunities \t get info about communities"},
 {CHECK_CONTINUITY, 0, "", "checkContinuity", OptionParser::Arg::None, "  --checkContinuity \t get info about continuity"},
 {RUNS, 0, "", "runs", OptionParser::Arg::Required, "  --runs=<NUMBER> \t set number of clusterer runs"},
 {SAVE_GRAPH, 0, "", "saveGraph", OptionParser::Arg::Required, "  --saveGraph=<PATH> \t write the graph to a file"},
 {PROGRESS, 0, "", "progress", OptionParser::Arg::None, "  --progress \t print progress bar"},
 {SUMMARY, 0, "", "summary", OptionParser::Arg::Required, "  --summary=<PATH> \t append summary as a .csv line to this file"},
 {UPDATE_THRESHOLD, 0, "", "updateThreshold", OptionParser::Arg::Required, "  --updateThreshold=<N> or --updateThreshold=auto \t number of updated nodes below which label propagation terminates - auto determines this automatically from the size of the graph"},
 {SAVE_CLUSTERINGS, 0, "", "saveClusterings", OptionParser::Arg::Required, "  --saveClusterings=<PATH> \t "},

 {UNKNOWN, 0,"" ,  ""   ,OptionParser::Arg::None, "\nExamples:\n"
                                            " TODO" },
 {0,0,0,0,0,0}
};


// MAIN FUNCTIONS






int main(int argc, char **argv) {
	std::cout << "=== NetworKit - Dynamic Community Detection === " << std::endl;

	Aux::Timer totalRuntime;
	totalRuntime.start();

	// ENABLE FLOATING POINT EXCEPTIONS (needs GNU extension, apparently only available on Linux)
#ifdef _GNU_SOURCE
	// feenableexcept(FE_ALL_EXCEPT);
#endif


	/// PARSE OPTIONS

	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	OptionParser::Stats  stats(usage, argc, argv);
	OptionParser::Option options[stats.options_max], buffer[stats.buffer_max];
	OptionParser::Parser parse(usage, argc, argv, options, buffer);

	if (parse.error())
	 return 1;

	if (options[HELP] || argc == 0) {
	 OptionParser::printUsage(std::cout, usage);
	 return 0;
	}

	for (OptionParser::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
	 std::cout << "Unknown option: " << opt->name << "\n";

	for (int i = 0; i < parse.nonOptionsCount(); ++i)
	 std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";




	// CONFIGURE LOGGING
#ifndef NOLOGGING
#ifndef NOLOG4CXX
	if (options[LOGLEVEL]) {
		Aux::configureLogging(options[LOGLEVEL].arg);
	} else {
		Aux::configureLogging();	// with default level
	}
#endif
#endif


	// CONFIGURE PARALLELISM
	omp_set_nested(1); // enable nested parallelism


	if (options[THREADS]) {
		// set number of threads
		int nThreads = std::atoi(options[THREADS].arg);
		setNumberOfThreads(nThreads);
	}


	// CONFIGURE OUTPUT
	if (options[PROGRESS]) {
		PRINT_PROGRESS = true;
	} else {
		PRINT_PROGRESS = false;
	}


	// TODO: how to do summary?
	if (options[SUMMARY]) {
		// check if file exists by trying to read from it
		std::ifstream summary(options[SUMMARY].arg);
		if (summary) {
			// file exists
		} else {
			// create it and write CSV header
			std::ofstream summary(options[SUMMARY].arg);
			summary << "threads;algo;revision;graph;running;eps;clusters;mod" << std::endl; // TODO: update header
		}
	}



	// RUN UNIT TESTS

	if (options[TESTS]) {

#ifndef NOGTEST
	   ::testing::InitGoogleTest(&argc, argv);
	   INFO("=== starting unit tests ===");
	   return RUN_ALL_TESTS();
#else
	   throw std::runtime_error("unit tests are excluded from build by the NOGTEST preprocessor directive");
#endif
	}


	// SET NUMBER OF CLUSTERER RUNS
	int runs = 1;
	if (options[RUNS]) {
		runs = std::atoi(options[RUNS].arg);
	}


	// determine update threshold / abort criterion for LabelPropagation
	count theta = 0;
	if (options[UPDATE_THRESHOLD]) {
		std::string updateThresholdArg = options[UPDATE_THRESHOLD].arg;
		theta = std::atoi(updateThresholdArg.c_str());
	} else {
		theta = 10;
	}


	// RUN PROGRAM

	// select a dynamic graph source
	DynamicGraphSource* source = NULL;
	DynamicDGSParser* sourceDGS  = NULL;

	if (options[SOURCE]) {
		std::string sourceString = options[SOURCE].arg;
		std::vector<std::string> sourceParts = Aux::StringTools::split(sourceString, ':');
		std::string sourceName = sourceParts[0];


		if (sourceName == "PseudoDynamic") {
			if (sourceParts.size() >= 2) {
				std::string graphFile = sourceParts[1];
				GraphReader* reader = new METISGraphReader();
				Graph G = reader->read(graphFile);
				source = new PseudoDynamic(G);
			} else {
				throw std::runtime_error("PseudoDynamic source needs a graph file path as argument");
			}
		} else if (sourceName == "DynamicBarabasiAlbertGenerator") {
			// TODO: Pass k as a parameter
			source = new DynamicBarabasiAlbertGenerator(2);

		} else if (sourceName == "DynamicPubWebGenerator") {
			// TODO: enable DynamicPubWebGenerator
		} else if (sourceName == "DGS") {
			std::string graphFile = sourceParts[1];
			sourceDGS = new DynamicDGSParser(graphFile);
			source = sourceDGS;
			//source = new DynamicDGSParser(graphFile);

		}
	} else {
		std::cout << "[ERROR]�--source option must be supplied" << std::endl;
		exit(1);
	}


	// select algorithms
	std::vector<DynamicCommunityDetector*> detectors;
	if (options[DETECTORS]){

		std::string detectorsFull = options[DETECTORS].arg;
		std::vector<std::string> detectorsString = Aux::StringTools::split(detectorsFull, '%');

		for (std::string detector : detectorsString) {
			std::vector<std::string> detectorParts = Aux::StringTools::split(detector, ':');
			std::string detectorName = detectorParts[0];
			std::string detectorArguments = detectorParts[1];
			if (detectorName == "TDynamicLabelPropagation") {
				if (detectorArguments == "Isolate") {
					detectors.push_back(new TDynamicLabelPropagation<Isolate>(theta));
				} else if (detectorArguments == "IsolateNeighbors") {
					detectors.push_back(new TDynamicLabelPropagation<IsolateNeighbors>(theta));
				} else {
					std::cout << "[ERROR] unknown detector argument: " << detectorArguments << std::endl;
					exit(1);
				}
			} else if (detectorName == "DynamicLabelPropagation") {

				if (detectorArguments == "Isolate") {
					detectors.push_back(new DynamicLabelPropagation(theta, "Isolate"));
				} else if (detectorArguments == "IsolateNeighbors") {
					detectors.push_back(new DynamicLabelPropagation(theta, "IsolateNeighbors"));
				} else {
					std::cout << "[ERROR] unknown detector argument: " << detectorArguments << std::endl;
					exit(1);
				}

			} else if (detectorName == "DynamicEnsemble") {
				DynamicEnsemble* dynamicEnsemble = new DynamicEnsemble();
				// parse params
				std::string ensembleFrontArg = Aux::StringTools::split(detectorArguments, '+').front();
				std::string finalClustererArg = Aux::StringTools::split(detectorArguments, '+').back();
				std::string ensembleSizeArg = Aux::StringTools::split(ensembleFrontArg, '*').front();
				std::string baseClustererArg = Aux::StringTools::split(ensembleFrontArg, '*').back();

				int ensembleSize = std::atoi(ensembleSizeArg.c_str());
				// 1. add base clusterers
				for (int i = 0; i < ensembleSize; i += 1) {
					Clusterer* base = NULL;
					if (baseClustererArg == "TDynamicLabelPropagation") {
						DynamicCommunityDetector* dynLP = new TDynamicLabelPropagation<Isolate>(theta);
						dynamicEnsemble->addBaseAlgorithm(*dynLP);
					} else {
						std::cout << "[ERROR] unknown base clusterer: " << baseClustererArg << std::endl;
						exit(1);
					}
					// TODO: dynamicEnsemble->addBaseClusterer(*base);
				}
				// 2. overlap algorithm
				Overlapper* overlap = new HashingOverlapper();
				dynamicEnsemble->setOverlapper(*overlap);
				// 3. Final Clusterer#
				Clusterer* final = NULL;
				if (finalClustererArg == "PLM") {
					final = new PLM("simple");
				} else if (finalClustererArg == "PLP") {
					final = new PLP(theta);
				} else {
					std::cout << "[ERROR] unknown final clusterer: " << finalClustererArg << std::endl;
					exit(1);
				}

				dynamicEnsemble->setFinalAlgorithm(*final);

				detectors.push_back((DynamicCommunityDetector*) dynamicEnsemble);

			} else {
				std::cout << "[ERROR] unknown detector name: " << detectorName << std::endl;
				exit(1);
			}
		} // end for detector arguments
		for (DynamicCommunityDetector* detector : detectors) {
			INFO("will add " << detector->toString() << " to setup");
		}
	} else {
		std::cout << "[ERROR] no community detector given" << std::endl;
		exit(1);
	}

	// static detector

	Clusterer* staticDetector = NULL;
	if (options[STATIC]) {
		std::string staticArg = options[STATIC].arg;
		std::string staticName = Aux::StringTools::split(staticArg, ':').front();
		std::string staticParams = Aux::StringTools::split(staticArg, ':').back();
		if (staticName == "PLP") {
			staticDetector = new PLP(theta);
		} else if (staticName == "PLM") {
			staticDetector = new PLM("simple");
		} else if (staticName == "EPP") {
			EPP* ensemblePre = new EPP();
			// parse params
			std::string ensembleFrontArg = Aux::StringTools::split(staticParams, '+').front();
			std::string finalClustererArg = Aux::StringTools::split(staticParams, '+').back();
			std::string ensembleSizeArg = Aux::StringTools::split(ensembleFrontArg, '*').front();
			std::string baseClustererArg = Aux::StringTools::split(ensembleFrontArg, '*').back();

			int ensembleSize = std::atoi(ensembleSizeArg.c_str());
			// 1. add base clusterers
			for (int i = 0; i < ensembleSize; i += 1) {
				Clusterer* base = NULL;
				if (baseClustererArg == "PLP") {
					base = new PLP(theta);
				} else {
					std::cout << "[ERROR] unknown base clusterer: " << baseClustererArg << std::endl;
					exit(1);
				}
				ensemblePre->addBaseClusterer(*base);
			}
			// 2. overlap algorithm
			Overlapper* overlap = new HashingOverlapper();
			ensemblePre->setOverlapper(*overlap);
			// 3. Final Clusterer
			Clusterer* final = NULL;
			if (finalClustererArg == "PLP") {
				final = new PLP();
			} else if (finalClustererArg == "PLM") {
				final = new PLM("balanced");
			} else {
				std::cout << "[ERROR] unknown final clusterer: " << finalClustererArg << std::endl;
				exit(1);
			}

			ensemblePre->setFinalClusterer(*final);

			staticDetector = ensemblePre;
		} else {
			std::cout << "[ERROR] unknown static detector: " << staticName << std::endl;
			exit(1);
		}
	}


	count tMax = 1e4;	//!< maximum time steps
	if (options[TMAX]) {
		tMax = std::stoi(options[TMAX].arg);
	}


	count deltaT = 1000; //!< detector run interval
	if (options[DELTAT]) {
		deltaT = std::stoi(options[DELTAT].arg);
	}



	INFO("creating setup with tMax=" << tMax << " and deltaT=" << deltaT);
	DynCDSetup* dynCDSetup = new DynCDSetup(*source, detectors, tMax, deltaT);

	if (options[STATIC]) {
		INFO("setting static detector " << staticDetector->toString());
		dynCDSetup->setStatic(staticDetector);
	}


	if (options[CHECK_COMMUNITIES]) {
		INFO("will check communities");
		dynCDSetup->checkModularity();
		dynCDSetup->checkNumberOfCommunities();
	}


	if (options[CHECK_CONTINUITY]) {
		INFO("will check continuity");
		dynCDSetup->checkContinuity();
	}

	for (int run = 0; run < runs; run++) {
		dynCDSetup->run();
	}


	if (options[SAVE_CLUSTERINGS]) {
		Clustering last;
		for (std::vector<Clustering> clusteringSequence : dynCDSetup -> dynamicClusteringTimelines) {
			last = clusteringSequence.back();
		}
		sourceDGS -> evaluateClusterings(options[SAVE_CLUSTERINGS].arg, last);
	}
	totalRuntime.stop();
	std::cout << "[EXIT] terminated normally after " << totalRuntime.elapsedTag() << std::endl;
	return 0;
}




