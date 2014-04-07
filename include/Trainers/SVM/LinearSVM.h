/**
 *  ==============================================================================
 *
 *          \file   Trainers/SVM/LinearSVM.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-06 21:41:18.444778
 *
 *  \Description:
 SVM (Pegasos-Linear)	/cl:LinearSVM	Fast primal-space stochastic gradient descent solver (with an optional projection step).
 S. Shalev-Shwartz, Y. Singer, and N. Srebro.  Pegasos:
 Primal Estimated sub-GrAdient SOlver for SVM. ICML-2007.	Projection step is off by default (resulting in a 1.5-2x speedup).
 It is advisable to pre-normalize or turn off normalization for sparse data. (or pre-normalize via /c CreateInstances if normalization does help)
 *  ==============================================================================
 */

#ifndef TRAINERS__S_V_M__LINEAR_S_V_M_H_
#define TRAINERS__S_V_M__LINEAR_S_V_M_H_

#include "random_util.h"
#include "ProgressBar.h"
#include "MLCore/IterativeTrainer.h"
#include "Prediction/Instances/Instances.h"
#include "Numeric/Vector/Vector.h"
namespace gezi {

	class LinearSVM : public IterativeTrainer
	{
	public:
		LinearSVM()
		{
			ParseArgs();
		}

		LinearSVM(int randSeed)
			:_rand(randSeed)
		{
			ParseArgs();
		}

		struct Arguments
		{
			int numIterations = 50000; //iter|Number of iterations
			Float lambda = 0.001; //lr|
			Float sampleRate = 0.001; //sr|Sampling rate
			int sampleSize = 1; //ss|Sampling size
			bool performProjection = false; //project|Perform projection to unit-ball
			bool noBias = false;
			bool calibrateOutput = true; //calibrate|
			string initialWeightsString = ""; //initweights|Initial weights and bias, comma-separated
			bool randomInitialWeights = false; //randweights|Randomize initial weights
			int featureNumThre = 1000; //fnt|if NumFeatures > featureNumThre use dense format 
			//��ʱ��֧��streaming ģʽ
			bool doStreamingTraining = false; //stream|Streaming instances training
		};

		void ParseArgs()
		{

		}

		virtual void Initialize(Instances& instances) override
		{
			numFeatures = instances.FeatureNum();

			if (_args.initialWeightsString.size() > 0)
			{
				LOG(INFO) << "Initializing weights and bias to " << _args.initialWeightsString;
				svec weightStr = split(_args.initialWeightsString, ',');
				if ((int)weightStr.size() == numFeatures + 1)
				{
					Fvec weightArr(numFeatures);
					for (int i = 0; i < numFeatures; i++)
						weightArr[i] = DOUBLE(weightStr[i]);
					_weights.Init(weightArr);
					_bias = DOUBLE(weightStr[numFeatures]);
				}
				else
				{
					LOG(WARNING) << "Could not inialize weights and bias from input use default";
				}
			}

			// weight initialization -- done unless we have initialized before
			if (_weights.length == 0)
			{
				// We want a dense vector, to prevent memory creation during training
				// unless we have a lot of features
				// TODO:   make a setting 
				_weights.SetLength(numFeatures);
				if (numFeatures <= _args.featureNumThre)
				{ //ʹ��dense��ʾ
					_weights.values.resize(numFeatures, 0);
					_weights.keepDense = true;
				}
				_bias = 0;

				// weights may be set to random numbers distributed uniformly on -1,1
				if (_args.randomInitialWeights)
				{
					for (int featureIdx = 0; featureIdx < numFeatures; featureIdx++)
					{
						_weights[featureIdx] = 2 * _rand.NextFloat() - 1;
					}
					if (!_args.noBias)
						_bias = 2 * _rand.NextFloat() - 1;
				}
			}

			// counters
			iteration = 0;
			numProcessedExamples = 0;
			numIterExamples = 0;

			// train clicks and errors
			numFalseNegTrain = 0;
			numFalsePosTrain = 0;
			numPosTrain = 0;
			numNegTrain = 0;

			// test clicks and errors
			numFalseNegTest = 0;
			numFalsePosTest = 0;
			numClicksTest = 0;
			numNonClicksTest = 0;

			VLOG(3) << "Initialized LinearSVM on " << numFeatures << " features";
		}

		virtual void BeginTrainingIteration() override
		{
			numProcessedExamples = 0;
			numIterExamples = 0;
			biasUpdate = 0;
		}

		/// Override the default training loop:   we need to pick random instances manually...
		virtual void InnerTrain(Instances& instances) override
		{
			_featureNames = instances.Schema.FeatureNames();
			ProgressBar pb(_args.numIterations);
			for (int iter = 0; iter < _args.numIterations; iter++)
			{
				pb.progress(iter);
				BeginTrainingIteration();

				if (_args.sampleSize == 0)
				{ // rate sampling
					for (int i = 0; i < data.Count() * _args.sampleRate; i++)
					{
						int idx = _rand.Next(data.Count());
						ProcessDataInstance(data[idx]);
					}
				}
				else
				{ // size sampling  ��ǰ������
					for (int i = 0; i < _args.sampleSize; i++)
					{
						int idx = _rand.Next(data.Count());
						ProcessDataInstance(data[idx]);
					}
				}

				bool wantMore = FinishDataIteration();
			}
			
			TrainingComplete();
		}

		/// Process a given training example.         /// Ŀǰû�õ� streaming��ʽ �������
		virtual bool ProcessTrainingInstance(InstancePtr instance) override
		{
			++numProcessedExamples;

			//if (calibrationIteration)
			//{
			//	return ProcessCalibrationInstance(instance);
			//}

			//if (normalizationIteration)
			//{
			//	return ProcessNormalizationInstance(instance);
			//}

			// train on this example or skip it?
			if (_args.sampleSize == 0)
			{ // rate sampling
				if (_rand.NextFloat() > _args.sampleRate)
					return true;
			}
			else
			{ // size sampling
				if (weightUpdates.Count() == _args.sampleSize)
				{
					// should we replace an existing update with this one? 
					Float toss = _rand.NextFloat();
					if (toss > ((Float)_args.sampleSize / numProcessedExamples))
						return true;
				}
			}

			// do an actual update (if any)
			return ProcessDataInstance(instance);
		}

		/// <summary>
		/// Called after last case is sent. Learner sets bMoreIterations to true if it wants another
		/// iteration of the data.  
		/// First does normalization iteration
		/// Then does training iteration
		/// Then does calibration iteration
		/// Then any further data is used for online training
		/// </summary>        
		virtual void FinishTrainingIteration(out bool bMoreIterations) override
		{
			if (calibrationIteration)
			{
				//FinishCalibrationIteration();
				bMoreIterations = false;
				return;
			}

			if (normalizationIteration)
			{
				//FinishNormalizationIteration();
				bMoreIterations = true;
				return;
			}

			FinishDataIteration(ref(bMoreIterations));
		}
	
		/// Observe an example and update weights if necessary
		bool ProcessDataInstance(InstancePtr instance)
		{
			++numIterExamples;

			// compute the update and update if needed     
			Float output = Margin(instance);
			Float trueOutput = (instance.IsPositive() ? 1 : -1);
			Float loss = output * trueOutput - 1;

			// record the update if there is a loss
			if (loss < 0)
			{
				Float currentBiasUpdate;
				Vector currentWeightUpdate;
				GetUpdate(output, trueOutput, instance, 
					ref(currentWeightUpdate), ref(currentBiasUpdate));

				if (_args.sampleSize == 0)
				{ // rate sampling     @TODO                                   
					/*if (weightsUpdate == null)
					{
						weightsUpdate = currentWeightUpdate;
					}
					else
					{
						weightsUpdate.Add(currentWeightUpdate);
					}
					biasUpdate += currentBiasUpdate;*/
				}
				else
				{ // pick a slot
					if (weightUpdates.size() < _args.sampleSize)
					{
						weightUpdates.emplace_back(currentWeightUpdate);
						biasUpdates.push_back(currentBiasUpdate);
					}
					else
					{ // need to replace random one
						int idxToReplace = _rand.Next(_args.sampleSize);
						weightUpdates[idxToReplace] = move(currentWeightUpdate);
						biasUpdates[idxToReplace] = currentBiasUpdate;
					}
				}
			}

			if (loss < 0)
			{ // update error statistics
				if ((output < 0 && instance.label > 0) || (output > 0 && instance.label == 0))
				{
					if (numProcessedExamples < numHoldoutExamples)
						if (output < 0)
							numFalseNegTrain++;
						else
							numFalsePosTrain++;
					else
						if (output < 0)
							numFalseNegTest++;
						else
							numFalsePosTest++;
				}
			}

			// update class distribution statistics
			if (numProcessedExamples < numHoldoutExamples)
				if (instance.label > 0)
					numPosTrain++;
				else
					numNegTrain++;
			else
				if (instance.label > 0)
					numClicksTest++;
				else
					numNonClicksTest++;

			return true;
		}

		void FinishDataIteration(bool& bMoreIterations)
		{
			++iteration;
			bMoreIterations = (iteration < _args.numIterations);

			if (numIterExamples > 0)
				ScaleWeights();

			// if more iterations, reset the counters and update learning rate if needed
			if (bMoreIterations)
			{
				numProcessedExamples = 0;
				numIterExamples = 0;
				numHoldoutExamples = 0;

				// train clicks and errors
				numFalseNegTrain = 0;
				numFalsePosTrain = 0;
				numPosTrain = 0;
				numNegTrain = 0;

				// test clicks and errors
				numFalseNegTest = 0;
				numFalsePosTest = 0;
				numClicksTest = 0;
				numNonClicksTest = 0;
			}

			// if calibration is needed, set it up
			if (bMoreIterations == false && _args.calibrateOutput)
			{
				bMoreIterations = true;
				calibrationIteration = true;
			}
		}

		Arguments& Args()
		{
			return _args;
		}

	protected:
	private:
		/// <summary>
		/// Return the raw margin from the decision hyperplane
		/// </summary>		
		Float Margin(InstancePtr& instance)
		{
			if (instance == lastMarginInstance)
			{
				return lastMargin;
			}
			else
			{
				//@TODO normalize and try to dense as TLC?
				lastMargin = _bias + dot(_weights, instance->features);
				lastMarginInstance = instance;
				return lastMargin;
			}
		}

		// <summary>
		/// Given an impression, and the output of the classifier, compute an update
		/// </summary>        
		void GetUpdate(Float output, Float trueOutput, Instance& instance,  
			Vector& gradient,Float& biasUpdate)
		{
			gradient = instance.features;
			biasUpdate = 0;

			// scale regret by weight
			trueOutput *= instance.weight;
			gradient.ScaleBy(trueOutput);
			biasUpdate = _args.noBias ? 0 : trueOutput;
		}

		/// <summary>
		/// Scale the weights at the end of the iteration
		/// </summary>
		void ScaleWeights()
		{
			if (_args.sampleSize == 0)
			{ // rate sampling
				ScaleWeightsSampled();
			}
			else
			{ // size sampling
				ScaleWeightsFixed();
			}

			lastMarginInstance = nullptr;
		}

		/// <summary>
		/// Scale the weights at the end of the iteration when we're sampling training instances
		/// </summary>
		private void ScaleWeightsSampled()
		{
			// w_{t+1/2} = (1-eta*lambda) w_t + eta/k * totalUpdate
			Float learningRate = 1 / (_args.lambda * iteration);
			_weights.ScaleBy(1 - learningRate * _args.lambda);

			if (weightsUpdate != null)
			{
				weightsUpdate.ScaleBy(learningRate / numIterExamples);
				_weights.Add(weightsUpdate);
			}

			_bias = (1 - learningRate * _args.lambda) * _bias + learningRate / numIterExamples * biasUpdate;

			// w_{t+1} = min{1, 1/sqrt(lambda)/|w_{t+1/2}|} * w_{t+1/2}
			if (_args.PerformProjection)
			{
				Float normalizer = 1 / (MathUtils.Sqrt(_args.lambda) * _weights.Norm());
				if (normalizer < 1)
				{
					_weights.ScaleBy(normalizer);
					//bias = bias * normalizer;
				}
			}
		}

		/// <summary>
		/// Scale the weights at the end of the iteration when we picked a random number of training instances
		/// </summary>
		private void ScaleWeightsFixed()
		{
			// add up the updates
			foreach(WritableVector nextUpdate in weightUpdates)
			{
				if (weightsUpdate == null)
				{
					weightsUpdate = nextUpdate;
				}
				else
				{
					weightsUpdate.Add(nextUpdate);
				}
			}

			// add up bias update
			foreach(Float bUpdate in biasUpdates)
			{
				biasUpdate += bUpdate;
			}
			ScaleWeightsSampled();
		}
	private:
		Arguments _args;

		/// <summary> Total number of features </summary>
		int numFeatures;
		/// <summary> Feature weights: weights for the last-seen training example </summary>
		Vector _weights;
		/// <summary> Prediction bias </summary>
		/// TODO: Note, I changed this also to mean the averaged bias. Should probably have two functions to
		///  make explicit whether you want the averaged or last bias. Same for weights.
		Float _bias;
		/// <summary>
		/// Cache last instance on which we computed margin so that we don't recompute
		/// </summary>
		Float lastMargin = 0;
		InstancePtr lastMarginInstance = nullptr;
		Random _rand;

		// number of processed examples and actual weight updates
		private uint64 numProcessedExamples = 0;
		private uint64 numIterExamples = 0;

		private int iteration = 0;
		private bool normalizationIteration = false;
		private bool calibrationIteration = false;

		// error counts
		private uint64 numFalsePosTrain = 0;
		private uint64 numFalseNegTrain = 0;
		private uint64 numPosTrain = 0;
		private uint64 numNegTrain = 0;

		// compute error separately after X examples
		private uint64 numHoldoutExamples = std::numeric_limits<uint64>::max();

		// testing statistics
		private uint64 numFalsePosTest = 0;
		private uint64 numFalseNegTest = 0;
		private uint64 numClicksTest = 0;
		private uint64 numNonClicksTest = 0;

		// history of weight updates
		vector<vector<Float> > weightHistory;

		Vector weightsUpdate;
		Float biasUpdate = 0;
		vector<Vector> weightUpdates;
		Fvec biasUpdates;
	};

}  //----end of namespace gezi

#endif  //----end of TRAINERS__S_V_M__LINEAR_S_V_M_H_