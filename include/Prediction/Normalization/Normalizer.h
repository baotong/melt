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
			NormalizeCore(vec);
		}

		virtual void NormalizeCore(Vector& vec)
		{

		}

		template<typename Func>
		void Normalize(Vector& vec, Func func)
		{
			if (vec.IsDense())
			{
				//NormalizeDense(vec, func);
				NormalizeDenseFast(vec, func);
			}
			else
			{
				//NormalizeSparse(vec, func);
				NormalizeSparseFast(vec, func);
			}
		}

		//@TODO �������tlc ȥ�ж�ĳЩλ�ñ��� offset 0 scale 1 ����Ҫ��func �ٶ����ж������� ֵ����
		//�������е����ݹ�ģ ��ʱ����Ҫ norm 1.2w 200���� 2ms
		//����޸� ��Ҫ��¼ ������Ҫscaleλ�õ�һ��list Ȼ��
		//��Ҫ����list ������ ��Ҫ�����ϡ�����ݴ���Ƚ��鷳 ������list�󽻼�
		template<typename Func>
		void NormalizeDense(Vector& vec, Func func)
		{
			vec.ForEachDense([&func](int index, Float& value)
			{
				func(index, ref(value));
			});
		}
	
		template<typename Func>
		void NormalizeSparse(Vector& vec, Func func)
		{
			Vector result(_featureNum);
			vec.ForEachAllSparse([&func, &result](int index, Float value)
			{
				Float val = value;
				func(index, ref(val));
				result.Add(index, val);
			});
			vec.Swap(result);
		}

		template<typename Func>
		void NormalizeDenseFast(Vector& vec, Func func)
		{
			for (int index : _scaleIndices)
			{
				func(index, vec.Values()[index]);
			}
		}

		template<typename Func>
		void NormalizeSparseFast(Vector& vec, Func func)
		{
			Vector result(_featureNum);
			int len = _scaleIndices.size();
			int len2 = vec.Values().size();
			
			Float val;
			int index, index2;
			int i = 0, j = 0;
			for (; i < len && j < len2;)
			{
				index = _scaleIndices[i];
				index2 = vec.Indices()[j];
				if (index == index2)
				{
				  val = vec.Values()[j];
					func(index, ref(val));
					result.Add(index, val);
					i++;
					j++;
				}
				else if (index < index2)
				{
					val = 0;
					func(index, ref(val));
					result.Add(index, val);
					i++;
				}
				else
				{
					j++;
				}
				for (��i < len��i++)
				{
					index = _scaleIndices[i];
					val = 0;
					func(index, ref(val));
					result.Add(index, val);
				}
			}
			
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
		
		void Prepare(const Instances& instances)
		{
			_featureNum = instances.FeatureNum();
			_featureNames = instances.FeatureNames();
			Begin();
			for (const InstancePtr& instance : instances)
			{
				Process(instance->features);
			}
			Finalize();
		}
		void Normalize(Instances& instances)
		{
			Prepare(instances);
#pragma omp parallel for //omp not work for foreach loop ? @TODO
			for (int i = 0; i < instances.Size(); i++)
			{
				Normalize(instances[i]->features);
			}
		}
	protected:
		int _featureNum;
		svec _featureNames;
		Float _lower = 0.0;
		Float _upper = 1.0;
		Float _range;
		ivec _scaleIndices;
		// if feature is out of bounds, threshold at 0/1, or return values below 0 and above 1?
		bool _trunct = false; //@TODO here or in MinMaxNormalizer GuassianNormalzier need it?
	private:
		
	};

	typedef shared_ptr<Normalizer> NormalizerPtr;
}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__NORMALIZER_H_
