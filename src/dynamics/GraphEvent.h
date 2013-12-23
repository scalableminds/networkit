/*
 * GraphEvent.h
 *
 *  Created on: 02.04.2013
 *      Author: cls
 */

#ifndef GRAPHEVENT_H_
#define GRAPHEVENT_H_

#include "../graph/Graph.h"

namespace NetworKit {



class GraphEvent {


public:

	enum Type {
		NODE_ADDITION,
		NODE_REMOVAL,
		EDGE_ADDITION,
		EDGE_REMOVAL,
		EDGE_WEIGHT_UPDATE,
		TIME_STEP
	};

	Type type;	//!< type of graph event
	node u; 				//!< first node parameter
	node v;					//!< second node parameter
	edgeweight w;			//!< edge weight parameter


	GraphEvent(Type type, node u = none, node v = none, edgeweight w = none);

	virtual ~GraphEvent();
};

} /* namespace NetworKit */
#endif /* GRAPHEVENT_H_ */