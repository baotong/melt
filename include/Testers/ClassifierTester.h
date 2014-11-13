/**
 *  ==============================================================================
 *
 *          \file   Testers/ClassifierTester.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-11-12 08:45:25.878259
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef TESTERS__CLASSIFIER_TESTER_H_
#define TESTERS__CLASSIFIER_TESTER_H_

#include "Testers/Tester.h"
#include "Utils/performance_evaluate.h"

namespace gezi {

	class ClassificationPrecisionRecall : public DatasetMetrics
	{
	public:
		virtual string LabelColumnName() override
		{
			return "True";
		}

		virtual vector<string> PerInstanceColumnNames() override
		{
			return vector<string>({ "Assigned", "Output", "Probability", "Log-loss" });
		}

		virtual Fvec ProcessInstance(Float label, Float prediction, Float probability, Float weight) override
		{
			vector<Float> results(4, std::numeric_limits<Float>::quiet_NaN());

			if (std::isnan(label))
			{
				numUnlabeledInstances++;
				return results;
			}

			if (std::isnan(prediction))
				return results;

			if (label > 0)
			{
				if (prediction > 0)
					numTruePos += weight;
				else
					numFalseNeg += weight;
			}
			else
			{
				if (prediction <= 0)
					numTrueNeg += weight;
				else
					numFalsePos += weight;
			}
			results[0] = (prediction > 0 ? 1 : 0);
			results[1] = prediction;

			Float currLogLoss;

			if (!std::isnan(probability))
			{
				if (label > 0)
				{
					currLogLoss = useLn ? -log(probability) : -log(probability) / log(2);
					numLogLossPositives += weight;
				}
				else
				{
					currLogLoss = useLn ? -log(1.0 - probability) : -log(1.0 - probability) / log(2);
					numLogLossNegatives += weight;
				}
			}
			else
			{
				currLogLoss = std::numeric_limits<Float>::quiet_NaN();
			}

			if (currLogLoss > logTolerance || std::isinf(currLogLoss))
				currLogLoss = logTolerance;
			logLoss += currLogLoss * weight;

			results[2] = probability;
			results[3] = currLogLoss;
			return results;
		}


	protected:
		virtual void Finish() override
		{
			accuracy = (numTrueNeg + numTruePos) / Float(numTruePos + numTrueNeg + numFalseNeg + numFalsePos);
			recallPos = (numTruePos + numFalseNeg > 0)
				? numTruePos / Float(numTruePos + numFalseNeg) : 0;
			precisionPos = (numTruePos + numFalsePos > 0)
				? numTruePos / Float(numTruePos + numFalsePos) : 0;
			recallNeg = (numTrueNeg + numFalsePos > 0)
				? numTrueNeg / Float(numTrueNeg + numFalsePos) : 0;
			precisionNeg = (numTrueNeg + numFalseNeg > 0)
				? numTrueNeg / Float(numTrueNeg + numFalseNeg) : 0;

			logLossReduction = 0;
			if (numLogLossPositives + numLogLossNegatives > 0)
			{
				logLoss = logLoss / Float(numLogLossPositives + numLogLossNegatives);

				Float priorPos = numLogLossPositives / Float(numLogLossPositives + numLogLossNegatives);
				priorLogLoss = gezi::entropy(priorPos, useLn);
				logLossReduction = 100 * (priorLogLoss - logLoss) / priorLogLoss;
			}
		}

		virtual void Print_(string prefix) override
		{
			avgLogLoss = logLoss;
			Float testPrior = (numTruePos + numFalseNeg) / (numTruePos + numTrueNeg + numFalseNeg + numFalsePos);
			Float testLogLoss = gezi::entropy(testPrior, useLn);

			if (trainPrior < 0)
			{
				trainPrior = testPrior;
				//fmt::print_line("\n\n*** Predictor did not carry a train prior, using test prior...");
			}

			if (numUnlabeledInstances > 0)
				fmt::print_line("\nEncountered {} unlabeled instances during testing", numUnlabeledInstances);

			fmt::print_line("\n{0}TEST POSITIVE RATIO:\t{1:.4f} ({2}/({2}+{3}))", prefix,
				1.0 * (numTruePos + numFalseNeg) / (numTruePos + numTrueNeg + numFalseNeg + numFalsePos),
				numTruePos + numFalseNeg, numFalsePos + numTrueNeg);
			//@TODO 是否有必要类似TLC predictor记录了训练数据的正负样本比例分布
			//if (trainPrior != -1)                          
			//	fmt::print_line("{0}TRAIN POSITIVE RATIO:\t{1}", prefix, trainPrior);

			fmt::print_line("\n{0}Confusion table:", prefix);
			fmt::print_line("         ||===============================||");
			fmt::print_line("         ||            PREDICTED          ||");
			fmt::print_line("  TRUTH  ||    positive    |   negative   || RECALL");
			fmt::print_line("         ||===============================||");

			string line = format(" positive||   {}", numTruePos);
			line = pad_right(line, 26);
			line = format("{} |    {}", line, numFalseNeg);
			line = pad_right(line, 42) + "|| ";
			line = line + format("{0:.4f} ({1}/{2})", recallPos, numTruePos, numTruePos + numFalseNeg);
			fmt::print_line(line);

			line = format(" negative||   {}", numFalsePos);
			line = pad_right(line, 26);
			line = format("{} |    {}", line, numTrueNeg);
			line = pad_right(line, 42) + "|| ";
			line = line + format("{0:.4f} ({1}/{2})", recallNeg, numTrueNeg, numTrueNeg + numFalsePos);
			fmt::print_line(line);

			fmt::print_line("         ||===============================||");
			line = " PRECISION " + format("{0:.4f} ({1}/{2})",
				precisionPos, numTruePos, numTruePos + numFalsePos);
			line = pad_right(line, 27);
			line = line + "  " + format("{0:.4f}({1}/{2})",
				precisionNeg, numTrueNeg, (numTrueNeg + numFalseNeg));
			fmt::print_line(line);

			fmt::print_line(prefix + "LOG LOSS/instance:\t\t{0:.4f}", avgLogLoss);
			fmt::print_line(prefix + "TEST-SET ENTROPY (prior LL/in):\t{0:.4f}", testLogLoss);
			fmt::print_line(prefix + "LOG-LOSS REDUCTION (RIG):\t{0:.4f}%", logLossReduction);
			fmt::print_line("\n" + prefix + "OVERALL 0/1 ACCURACY:	{0:.4f} ({1}/{2})", accuracy,
				(numTrueNeg + numTruePos),
				(numTruePos + numTrueNeg + numFalseNeg + numFalsePos));
			fmt::print_line("POS.PRECISION:		{:.4f}", precisionPos);
			fmt::print_line("PPOS.RECALL:		{:.4f}", recallPos);
			fmt::print_line("POS.PRECISION:		{:.4f}", precisionNeg);
			fmt::print_line("PPOS.RECALL:		{:.4f}", recallNeg);
			//if (trainPrior > 0)
			//{
			//	Float trainLogLoss = gezi::cross_entropy(testPrior, trainPrior, useLn);
			//	fmt::print_line("   " + prefix + "TRAIN-SET PRIOR LL/inst (train-test cross-entropy):\t\t\t{0}", trainLogLoss);
			//	fmt::print_line("   " + prefix + "TRAIN-SET LL REDUCTION (trainRIG diagnostic, *not* for comparisons):\t{0}%", 100.0 * (trainLogLoss - avgLogLoss) / trainLogLoss);
			//}
		}

	protected:
		Float numTruePos = 0, numFalsePos = 0, numTrueNeg = 0, numFalseNeg = 0;
		int64 numUnlabeledInstances = 0;
		Float logLoss = 0, priorLogLoss = 0;
		// need to keep separate in case NaNs in probability
		Float numLogLossPositives, numLogLossNegatives;
		Float trainPrior = -1;

		bool useLn = false;

		//// cutoff for log-loss tolerances (to prevent infinite loss)
		//[Argument(ArgumentType.AtMostOnce, HelpText = "Confidence threshold (log-loss limit)", ShortName = "logtol", DefaultValue = 30.0)]
		//public Float logTolerance = 30.0;
		Float logTolerance = 30.0;

		Float accuracy, precisionPos, recallPos, precisionNeg, recallNeg, avgLogLoss, logLossReduction;
	};

	class ClassificationAUC : public DatasetMetrics
	{
	public:
		virtual string LabelColumnName() override
		{
			return "True";
		}

		virtual vector<string> PerInstanceColumnNames() override
		{
			//return vector<string>({ "Test", "Haha"});
			return vector<string>();
		}

		virtual Fvec ProcessInstance(Float label, Float prediction, Float probability, Float weight) override
		{
			_results.push_back(std::make_tuple(label, prediction, weight));
			return vector<Float>();
		}

	protected:
		virtual void Finish() override
		{
			_auc = gezi::auc(_results);
		}

		virtual void Print_(string prefix) override
		{
			fmt::print_colored(fmt::RED, prefix + "AUC:		[{0:.4}]", _auc);
			fmt::print("\n");
		}
	private:
		vector<std::tuple<int, Float, Float> > _results;
		Float _auc = 0.;
	};


	class ClassifierTester : public Tester
	{
	public:
		virtual PredictionKind GetPredictionKind()
		{
			return PredictionKind::BinaryClassification;
		}
		virtual vector<DatasetMetricsPtr> ConstructDatasetMetrics()
		{
			return vector<DatasetMetricsPtr>({
				make_shared<ClassificationPrecisionRecall>(),
				make_shared<ClassificationAUC>()
			});
		}
	};

}  //----end of namespace gezi

#endif  //----end of TESTERS__CLASSIFIER_TESTER_H_
