/**
 *  ==============================================================================
 *
 *          \file   Predictors/LinearPredictor.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-07 21:50:18.354006
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PREDICTORS__LINEAR_PREDICTOR_H_
#define PREDICTORS__LINEAR_PREDICTOR_H_

#include "MLCore/Predictor.h"
#include "Prediction/Normalization/Normalizer.h"
#include "Prediction/Calibrate/Calibrator.h"
namespace gezi {

	class LinearPredictor : public Predictor
	{
	public:
		LinearPredictor() = default;

		//ѵ������ʹ�ù���
		LinearPredictor(const Vector& weights, Float bias,
			NormalizerPtr normalizer, CalibratorPtr calibrator,
			const svec& featureNames)
			:Predictor(normalizer, calibrator),
			_weights(weights), _bias(bias),
			_featureNames(featureNames)
		{

		}

		LinearPredictor(string path)
		{

		}

		virtual string Name() override
		{
			return "LinearPredictor";
		}

		virtual void Save(string path) override
		{
			Predictor::Save(path);
			string modelFile = path + "/model";
			serialize_util::save(*this, modelFile);
		}

		virtual void Load(string path) override
		{
			Predictor::Load(path);
			string modelFile = path + "/model";
			serialize_util::load(*this, modelFile);
		}

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<Predictor>(*this);
			ar & _weights;
			ar & _bias;
			ar & _featureNames;
		}

	protected:
		virtual Float Margin(const Vector& features) override
		{
			return _bias + dot(_weights, features);
		}
	private:
		Vector _weights;
		Float _bias;
		svec _featureNames;
	};

}  //----end of namespace gezi

#endif  //----end of PREDICTORS__LINEAR_PREDICTOR_H_
