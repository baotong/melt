/**
 *  ==============================================================================
 *
 *          \file   calibrate.cc
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-11 01:03:08.334409
 *
 *  \Description: ���� true output ��� true output probability
 *
 *  ==============================================================================
 */

#define private public
#define protected public
#include "common_util.h"
#include "Prediction/Calibrate/SigmoidCalibrator.h"
using namespace std;
using namespace gezi;

DEFINE_int32(level, 0, "min log level");
DEFINE_string(type, "simple", "");
DEFINE_bool(perf, false, "");
DEFINE_int32(num, 1, "");
DEFINE_string(in, "", "input file");
DEFINE_string(out, "", "output file");

void run(string infile, string outfile)
{
	SigmoidCalibrator calibrator;
	ifstream ifs(infile);
	string line;
	dvec labels, outputs;
	while (getline(ifs, line))
	{
		string label_, output_;
		split(line, ' ', label_, output_);
		double label = DOUBLE(label_);
		double output = DOUBLE(output_);
		labels.push_back(label);
		outputs.push_back(output);
		calibrator.ProcessTrainingExample(output, label > 0, 1);
	}
	calibrator.FinishTraining();
	{
		ofstream ofs(outfile);
		ofs << "True\tOutput\tProbability" << endl;
		for (size_t i = 0; i < labels.size(); i++)
		{
			double prob = calibrator.PredictProbability(outputs[i]);
			ofs << labels[i] << "\t" << outputs[i] << "\t" << prob << endl;
		}
	}
	{
		ofstream ofs("sigmoid_params.txt");
		ofs << calibrator.paramA << "\t" << calibrator.paramB << endl;
	}
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();
	int s = google::ParseCommandLineFlags(&argc, &argv, false);
	if (FLAGS_log_dir.empty())
		FLAGS_logtostderr = true;
	FLAGS_minloglevel = FLAGS_level;

	run(argv[s], argv[s+1]);

	return 0;
}
