/*
 * Conductance.h
 *
 *  Created on: 26.02.2014
 *      Author: Henning
 */

#ifndef CONDUCTANCE_H_
#define CONDUCTANCE_H_

#include <networkit/community/QualityMeasure.hpp>
#include <networkit/community/EdgeCut.hpp>
#include <networkit/auxiliary/NumericTools.hpp>

namespace NetworKit {

/**
 * @ingroup community
 * Compute conductance of a 2-partition, i.e. cut size over volume of smaller set (smaller in
 * terms of volume).
 */
class Conductance: public QualityMeasure {
public:
    /**
     * @return Conductance of 2-partition @a zeta in graph @a G.
     * Requires cluster IDs to be either 0 or 1.
     */
    virtual double getQuality(const Partition& zeta, const Graph& G);
};

} /* namespace NetworKit */
#endif /* CONDUCTANCE_H_ */
