/**
 *  ==============================================================================
 *
 *          \file   gbdt_predict.cc
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-14 18:27:45.210850
 *
 *  \Description:
 *
 *  ==============================================================================
 */
#define _DEBUG
#define private public
#define protected public
#include "common_util.h"
#include "MLCore/PredictorFactory.h"
#include "Predictors/GbdtPredictor.h"
using namespace std;
using namespace gezi;

DEFINE_int32(vl, 0, "vlog level, if you set to 3 you can see each tree score");

DEFINE_bool(pts, false, "print trees scores");
DEFINE_int32(n, -1, "print num trees scores if pts == true, if num < 0 print all trees scores");
DEFINE_bool(r, false, "reverse: show trees from -.. to + .. if reverse == true");

DEFINE_bool(s, true, "sortByGain|sort==true, will print by abs gain per feature, otherwise will print by feature id");
DEFINE_int32(t, -1, "tree index to print");
DEFINE_int32(si, 0, "startIndex| start from 0 default or start from 1 like libsvm");

DEFINE_string(m, "model", "modelPath");
DEFINE_string(f, "", "featureStr");

void run(string feature, string modelPath)
{
	//��Ϊ����_DEBUG GbdtPredictor��debugNodes�������Load��core  @TODO,��������ֻ��ֱ����GbdtPredictor�ˣ�GbdtRegression �� GbdtRankΨһ�������� PredictorKind��һ����Ԥ���
	//probalibity��outputһ��
	//auto predictor = PredictorFactory::LoadPredictor(modelPath);
	GbdtPredictor predictor(modelPath);
	int startIndex = FLAGS_si;
	Vector fe(feature, startIndex);
	double predict;
	double probablity = predictor.Predict(fe, predict);
	string predictorName = read_file(modelPath + "/model.name.txt");
	VLOG(0) << "predictorName:" << predictorName << " -- Notice for non gbdt binary classification model, proababilty is meaning less";
	if (predictorName != "gbdt" && predictorName != "fastrank" && predictorName != "fr")
	{
		probablity = predict;
	}
	Pval2(predict, probablity);

	bool printTreesScores = FLAGS_pts;
	bool sort = FLAGS_s;
	bool reverse = FLAGS_r;
	int maxTrees = FLAGS_n;

	//--------------------��ӡ�������ĵ÷�
	if (printTreesScores)
	{
		VLOG(0) << "Print Trees Scores, sortByScore: " << sort << " revrese: " << reverse << " maxTreesPrint: " << maxTrees;
		predictor.PrintTreeScores(sort, reverse, maxTrees);
		return;
	}

	//-------------------��ӡĳһ����
	int treeIndex = FLAGS_t;
	if (treeIndex >= 0)
	{
		VLOG(0) << "Print tree: " << treeIndex;
		predictor.Trees()[treeIndex].Print(fe);
		return;
	}

	//--------------------��ӡ����Ԥ���������Ҫ��(���)
	{
		VLOG(0) << "Per feature gain for this predict, sortByGain: " << sort << endl
			<< predictor.ToGainSummary(fe, sort);
		Pval2(predict, probablity);
	}
}



int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();
	google::SetVersionString(get_version());
	int s = google::ParseCommandLineFlags(&argc, &argv, false);
	if (FLAGS_log_dir.empty())
		FLAGS_logtostderr = true;
	if (FLAGS_v == 0)
		FLAGS_v = FLAGS_vl;

	run(FLAGS_f, FLAGS_m);

	return 0;
}

