
#include "common_util.h"
#include "Run/Melt.h"

DEFINE_string(c, "cv", "command: Options are: Train, Test, CV (cross validation), TrainTest, FeatureSelection, CreateInstances, Norm, Check");
DEFINE_string(cl, "", "classifierName: Classifier to use");
DEFINE_string(cls, "", "classifierSettings: Classifier settings");

//---------------cross validation 
DEFINE_int32(k, 5, "numFolds: Number of folds in k-fold cross-validation");
DEFINE_int32(nr, 1, "numRuns: numRuns: Number of runs of cross-validation");
DEFINE_bool(strat, false, "Stratify instances during cross-validation (instances with same name up to '|' get assigned to same fold");
DEFINE_bool(foldSeq, false, "foldSequence: instances sequentially during cross - validation");

//========== MODIFYING TRAIN/TEST DATA: learning curves, stratifying, skipping certain instances
//===============================================================================================
// learning curve experiments are done via trainProportion
DEFINE_double(tp, 1.0, "trainProportion: Fraction of training data to use (if < 1.0), or max training items");

DEFINE_int32(rs, 0, "0 means random, 1 means can reproduce | randSeed: controls wether the expermient can reproduce");
DEFINE_int32(nt, 0, "num of threads, default 0 means use threads num according to processor num");

DEFINE_string(i, "", "datafile: Input data file used for train or cross validation, you can also put data file just after exe like: ./melt datafile");
DEFINE_string(o, "", "outfile: specified output file(not modelfile)");
DEFINE_string(nf, "", "normalzierfile: specified the output normalzier text");
DEFINE_string(test, "", "testDatafile: Data file used for test");
DEFINE_string(valid, "", "Data file used for training validation (with IValidatingPredictor classifiers)");

DEFINE_string(m, "", "modelfile: Model file to save (for Train) or load (for Test)");
DEFINE_string(mc, "", "modelfileCode: Model file to save in C++ code (for Train or TrainTest)");
DEFINE_string(mt, "", "modelfileText:  Model file to save in text format (for Train or TrainTest");

DEFINE_bool(norm, true, "Normalize features");
DEFINE_string(normalizer, "MinMax", "Which normalizer?");

namespace gezi {
	void Melt::ParseArguments()
	{
		_cmd.command = FLAGS_c;
		_cmd.classifierName = FLAGS_cl;
		_cmd.classifierSettings = FLAGS_cls;
		_cmd.datafile = FLAGS_i;
		_cmd.outfile = FLAGS_o;
		_cmd.normalizerfile = FLAGS_nf;
		_cmd.testDatafile = FLAGS_test;
		_cmd.validationDatafile = FLAGS_valid;
		_cmd.modelfile = FLAGS_m;
		_cmd.modelfileCode = FLAGS_mc;
		_cmd.modelfileText = FLAGS_mt;
		_cmd.numFolds = FLAGS_k;
		_cmd.numRuns = FLAGS_nr;
		_cmd.randSeed = FLAGS_rs;
		_cmd.stratify = FLAGS_strat;
		_cmd.numThreads = FLAGS_nt;
		_cmd.foldsSequential = FLAGS_foldSeq;
		_cmd.trainProportion = FLAGS_tp;

		_cmd.normalizeFeatures = FLAGS_norm;
		_cmd.normalizerName = FLAGS_normalizer;
	}
} //end of namespace gezi