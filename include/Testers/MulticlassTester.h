/**
 *  ==============================================================================
 *
 *          \file   Testers/MulticlassTester.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-11-12 08:45:50.077699
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef TESTERS__MULTICLASS_TESTER_H_
#define TESTERS__MULTICLASS_TESTER_H_
#include "Tester.h"
namespace gezi {

class MulticlassTester : public Tester
{
public:
	virtual PredictionKind GetPredictionKind()
	{
		return PredictionKind::MultiClassClassification;
	}
	virtual vector<DatasetMetricsPtr> ConstructDatasetMetrics()
	{
		return vector<DatasetMetricsPtr>();
	}
protected:
private:

};

}  //----end of namespace gezi

#endif  //----end of TESTERS__MULTICLASS_TESTER_H_
