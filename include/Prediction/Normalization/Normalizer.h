/**
 *  ==============================================================================
 *
 *          \file   Prediction/Normalization/Normalizer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-01 17:07:28.184018
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PREDICTION__NORMALIZATION__NORMALIZER_H_
#define PREDICTION__NORMALIZATION__NORMALIZER_H_
#include "common_util.h"
#include "Prediction/Instances/Instances.h"
namespace gezi {

	//@TODO Ϊ���������Feature��ʹ�ã� ��Ϊģ���� ��������Feature���Ϊ�̳�Vector ?
	//����Feature��̳�Vector ����Vectorд���ֿ�index �� value ������dense��ʾ���Լ����������򼴿�
	//dense��ʾֻ��dump feature ������ ���Ը�д�� ����ForEachNonZero
	class Normalizer
	{
	public:
		Normalizer()
			:_range(_upper - _lower)
		{

		}

		/// Process next initialization example
		virtual void Process(const Vector& vec)
		{

		}

		//TLC �Ƚϸ��� ���Ҹо���bug  ������dense sparse��ת������ 
		//ͬʱ��norm֮�󸲸�ԭ��� �����Ҫ 
		void Normalize(Vector& vec)
		{
			if (vec.IsDense())
			{
				NormalizeDense(vec);
			}
			else
			{
				NormalizeSparse(vec);
			}
		}

		virtual void NormalizeDense(Vector& vec)
		{

		}

		virtual void NormalizeSparse(Vector& vec)
		{

		}

		template<typename Func>
		void NormalizeDenseCore(Vector& vec, Func func)
		{
			vec.ForEachDense([&func](int index, double& value)
			{
				value = func(index, value);
			});
		}

	
		template<typename Func>
		void NormalizeSparseCore(Vector& vec, Func func)
		{
			Vector result(_featureNum);
			vec.ForEachAllSparse([&func, &result](int index, double value)
			{
				value = func(index, value);
				result.Add(index, value);
			});
			vec.Swap(result);
		}

		/// Begin iterating through initialization examples
		virtual void Begin()
		{

		}

		/// Finish processing initialization examples; ready to normalize
		virtual void Finalize()
		{

		}
		
		void Normalize(Instances& instances)
		{
			_featureNum = instances.FeatureNum();
			Begin();
			for (InstancePtr& instance : instances)
			{
				Process(instance->features);
			}
			Finalize();
			for (InstancePtr& instance : instances)
			{
				Normalize(instance->features);
			}
		}
	protected:
		int _featureNum;
		Float _lower = 0.0;
		Float _upper = 1.0;
		Float _range;
	private:
		// if feature is out of bounds, threshold at 0/1, or return values below 0 and above 1?
		bool _trunct = false; //@TODO here or in MinMaxNormalizer GuassianNormalzier need it?
	};

	typedef shared_ptr<Normalizer> NormalizerPtr;
}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__NORMALIZER_H_
