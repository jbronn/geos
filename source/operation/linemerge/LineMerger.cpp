/**********************************************************************
 * $Id$
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2005-2006 Refractions Research Inc.
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <geos/operation/linemerge/LineMerger.h>
#include <geos/operation/linemerge/LineMergeDirectedEdge.h>
#include <geos/operation/linemerge/EdgeString.h>
#include <geos/planargraph/DirectedEdge.h>
#include <geos/planargraph/Edge.h>
#include <geos/planargraph/Node.h>
#include <geos/geom/GeometryComponentFilter.h>
#include <geos/geom/LineString.h>

#include <cassert>
#include <functional>
#include <vector>

using namespace std;
using namespace geos::planargraph;
using namespace geos::geom;

#ifndef GEOS_DEBUG
#define GEOS_DEBUG 0
#endif

namespace geos {
namespace operation { // geos.operation
namespace linemerge { // geos.operation.linemerge

void
LineMerger::add(vector<Geometry*> *geometries)
{
	for(unsigned int i=0, n=geometries->size(); i<n; i++) {
		Geometry *geometry=(*geometries)[i];
		add(geometry);
	}
}

LineMerger::LineMerger():
	mergedLineStrings(NULL),
	factory(NULL)
{
}

LineMerger::~LineMerger()
{
	for (unsigned int i=0, n=edgeStrings.size(); i<n; ++i) {
		delete edgeStrings[i];
	}
}


struct LMGeometryComponentFilter: public GeometryComponentFilter {
	LineMerger *lm;

	LMGeometryComponentFilter(LineMerger *newLm): lm(newLm) {}

	void filter(const Geometry *geom) {
		const LineString *ls = dynamic_cast<const LineString *>(geom);
		if ( ls ) lm->add(ls);
	}
};


/**
 * Adds a Geometry to be processed. May be called multiple times.
 * Any dimension of Geometry may be added; the constituent linework will be
 * extracted.
 */  
void
LineMerger::add(const Geometry *geometry)
{
	LMGeometryComponentFilter lmgcf(this);
	geometry->applyComponentFilter(lmgcf);
}

void
LineMerger::add(const LineString *lineString)
{
	if (factory==NULL) factory=lineString->getFactory();
	graph.addEdge(lineString);
}

void
LineMerger::merge()
{
	if (mergedLineStrings!=NULL) return;

	buildEdgeStringsForObviousStartNodes();
	buildEdgeStringsForIsolatedLoops();

	unsigned numEdgeStrings = edgeStrings.size();
	mergedLineStrings=new vector<LineString*>(numEdgeStrings);
	for (unsigned int i=0; i<numEdgeStrings; ++i)
	{
		EdgeString *edgeString=edgeStrings[i];
		(*mergedLineStrings)[i]=edgeString->toLineString();
	}    
}

void
LineMerger::buildEdgeStringsForObviousStartNodes()
{
	buildEdgeStringsForNonDegree2Nodes();
}

void
LineMerger::buildEdgeStringsForIsolatedLoops()
{
	buildEdgeStringsForUnprocessedNodes();
}  

void
LineMerger::buildEdgeStringsForUnprocessedNodes()
{
#if GEOS_DEBUG
	cerr<<__FUNCTION__<<endl;
#endif
	vector<Node*> *nodes=graph.getNodes();
	for (unsigned int i=0; i<nodes->size(); ++i) {
		Node *node=(*nodes)[i];
#if GEOS_DEBUG
		cerr<<"Node "<<i<<": "<<*node<<endl;
#endif
		if (!node->isMarked()) { 
			assert(node->getDegree()==2);
			buildEdgeStringsStartingAt(node);
			node->setMarked(true);
#if GEOS_DEBUG
			cerr<<" setMarked(true) : "<<*node<<endl;
#endif
		}
	}
	delete nodes;
}

void
LineMerger::buildEdgeStringsForNonDegree2Nodes()
{
#if GEOS_DEBUG
	cerr<<__FUNCTION__<<endl;
#endif
	vector<Node*> *nodes=graph.getNodes();
	unsigned int size=nodes->size();
	for (unsigned int i=0; i<size; i++) {
		Node *node=(*nodes)[i];
#if GEOS_DEBUG
		cerr<<"Node "<<i<<": "<<*node<<endl;
#endif
		if (node->getDegree()!=2) { 
			buildEdgeStringsStartingAt(node);
			node->setMarked(true);
#if GEOS_DEBUG
			cerr<<" setMarked(true) : "<<*node<<endl;
#endif
		}
	}
	delete nodes;
}

void
LineMerger::buildEdgeStringsStartingAt(Node *node)
{
	vector<planargraph::DirectedEdge*> &edges=node->getOutEdges()->getEdges();
	unsigned int size = edges.size();
	for (unsigned int i=0; i<size; i++)
	{
		assert(dynamic_cast<LineMergeDirectedEdge*>(edges[i]));
		LineMergeDirectedEdge *directedEdge=\
			static_cast<LineMergeDirectedEdge*> (edges[i]);
		if (directedEdge->getEdge()->isMarked()) {
			continue;
		}
		edgeStrings.push_back(buildEdgeStringStartingWith(directedEdge));
	}
}

EdgeString*
LineMerger::buildEdgeStringStartingWith(LineMergeDirectedEdge *start)
{    
	EdgeString *edgeString = new EdgeString(factory);
	LineMergeDirectedEdge *current=start;
	do {
		edgeString->add(current);
		current->getEdge()->setMarked(true);
		current=current->getNext();      
	} while (current!=NULL && current!=start);
	return edgeString;
}

/**
 * Returns the LineStrings built by the merging process.
 */
vector<LineString*>*
LineMerger::getMergedLineStrings()
{
	merge();
	return mergedLineStrings;
}

} // namespace geos.operation.linemerge
} // namespace geos.operation
} // namespace geos

/**********************************************************************
 * $Log$
 * Revision 1.14  2006/03/22 10:13:54  strk
 * opLinemerge.h split
 *
 * Revision 1.13  2006/03/21 21:42:54  strk
 * planargraph.h header split, planargraph:: classes renamed to match JTS symbols
 *
 **********************************************************************/
 

