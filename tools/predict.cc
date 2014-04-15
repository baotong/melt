/**
 *  ==============================================================================
 *
 *          \file   predict.cc
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-14 18:07:22.612175
 *
 *  \Description:  ����ĳ��ģ�� ���û������һ������ ����Ԥ����
 *
 *  ==============================================================================
 */

#define private public
#define protected public
#include "common_util.h"

#include "Predictors/PredictorFactory.h"

using namespace std;
using namespace gezi;

DEFINE_int32(level, 0, "min log level");

void run(string modelPath, string feature)
{
	auto predictor = PredictorFactory::LoadPredictor(modelPath);
	Vector fe(feature);
	double out;
	double probablity = predictor->Predict(fe, out);
	Pval2(out, probablity);
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();
	int s = google::ParseCommandLineFlags(&argc, &argv, false);
	if (FLAGS_log_dir.empty())
		FLAGS_logtostderr = true;
	FLAGS_minloglevel = FLAGS_level;

	string modelPath = argv[s];
	string feature;
	if (s + 1 < argc)
		feature = argv[s + 1];
	run(modelPath, feature);

	return 0;
}