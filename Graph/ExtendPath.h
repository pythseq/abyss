#ifndef _EXTENDPATH_H_
#define _EXTENDPATH_H_

#include "Graph/Path.h"
#include "Common/UnorderedSet.h"
#include "Common/UnorderedMap.h"
#include "Common/Hash.h"
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <cassert>
#include <cstdio>
#include <iostream>

/**
 * The result of attempting to extend a path.
 */
enum PathExtensionResult {
	/** path could not be extended because of a dead end */
	DEAD_END,
	/** path could not be extended because of a branching point */
	BRANCHING_POINT,
	/** path could not be extended because of a cycle */
	CYCLE,
	/** path could not be extended because of caller-specified length limit */
	LENGTH_LIMIT,
	/** path was extended up to a dead end */
	EXTENDED_TO_DEAD_END,
	/** path was extended up to a branching point */
	EXTENDED_TO_BRANCHING_POINT,
	/** path was extended up to a cycle */
	EXTENDED_TO_CYCLE,
	/** path was extended up to caller-specified length limit */
	EXTENDED_TO_LENGTH_LIMIT
};

/**
 * Translate path extension result code to a string.
 */
static inline const char* pathExtensionResultStr(PathExtensionResult result)
{
	switch(result) {
	case DEAD_END:
		return "DEAD_END";
	case BRANCHING_POINT:
		return "BRANCHING_POINT";
	case CYCLE:
		return "CYCLE";
	case LENGTH_LIMIT:
		return "LENGTH_LIMIT";
	case EXTENDED_TO_DEAD_END:
		return "EXTENDED_TO_DEAD_END";
	case EXTENDED_TO_BRANCHING_POINT:
		return "EXTENDED_TO_BRANCHING_POINT";
	case EXTENDED_TO_CYCLE:
		return "EXTENDED_TO_CYCLE";
	case EXTENDED_TO_LENGTH_LIMIT:
		return "EXTENDED_TO_LENGTH_LIMIT";
	default:
		assert(false);
	}
}

/**
 * Return true if the path extension result code indicates
 * that the path was successfully extended by one or more nodes.
 */
static inline bool pathExtended(PathExtensionResult result)
{
	switch(result) {
	case DEAD_END:
	case BRANCHING_POINT:
	case CYCLE:
	case LENGTH_LIMIT:
		return false;
	default:
		return true;
	}
	assert(false);
}

/**
 * The result of attempting to extend a path
 * by a single neighbouring vertex.
 */
enum SingleExtensionResult {
	SE_DEAD_END,
	SE_BRANCHING_POINT,
	SE_EXTENDED
};

/**
 * Return true if there is a path of at least depthLimit vertices
 * that extends from given vertex u, otherwise return false.
 * Implemented using a bounded depth first search.
 *
 * @param start starting vertex for traversal
 * @param dir direction for traversal (FORWARD or REVERSE)
 * @param depth depth of current vertex u
 * @param depthLimit maximum depth to probe
 * @param g graph to use for traversal
 * @param visited vertices that have already been visited by the DFS
 * @return true if at least one path with length >= len
 * extends from v in direction dir, false otherwise
 */
template <class Graph>
static inline bool lookAhead(
	const typename boost::graph_traits<Graph>::vertex_descriptor& u,
	Direction dir, unsigned depth, unsigned depthLimit,
	unordered_set< typename boost::graph_traits<Graph>::vertex_descriptor,
	hash<typename boost::graph_traits<Graph>::vertex_descriptor> >& visited, const Graph& g)
{
    typedef typename boost::graph_traits<Graph>::vertex_descriptor V;
    typedef typename boost::graph_traits<Graph>::out_edge_iterator OutEdgeIter;
    typedef typename boost::graph_traits<Graph>::in_edge_iterator InEdgeIter;

	OutEdgeIter oei, oei_end;
	InEdgeIter iei, iei_end;

	visited.insert(u);
	if (depth == depthLimit)
		return true;

	if (dir == FORWARD) {
		for (boost::tie(oei, oei_end) = out_edges(u, g);
			oei != oei_end; ++oei) {
			const V& v = target(*oei, g);
			if (visited.find(v) == visited.end()) {
				if(lookAhead(v, dir, depth+1, depthLimit, visited, g))
					return true;
			}
		}
	} else {
		assert(dir == REVERSE);
		for (boost::tie(iei, iei_end) = in_edges(u, g);
			 iei != iei_end; ++iei) {
			const V& v = source(*iei, g);
			if (visited.find(v) == visited.end()) {
				if(lookAhead(v, dir, depth+1, depthLimit, visited, g))
					return true;
			}
		}
	}

	return false;
}

/**
 * Return true if there is a path of at least 'depth' vertices
 * that extends from given vertex v, otherwise return false.
 * Implemented using a bounded depth first search.
 *
 * @param start starting vertex for traversal
 * @param dir direction for traversal (FORWARD or REVERSE)
 * @param depth length of path to test for
 * @param g graph to use for traversal
 * @return true if at least one path with length >= len
 * extends from v in direction dir, false otherwise
 */
template <class Graph>
static inline bool lookAhead(
	const typename boost::graph_traits<Graph>::vertex_descriptor& start,
	Direction dir, unsigned depth, const Graph& g)
{
	typedef typename boost::graph_traits<Graph>::vertex_descriptor V;
	unordered_set< V, hash<V> > visited;
	return lookAhead(start, dir, 0, depth, visited, g);
}

/**
 * Return neighbour vertices that begin branches that are longer than trimLen.
 *
 * @param u root vertex
 * @param dir direction for neighbours (FORWARD or REVERSE)
 * @param g graph
 * @param trimLen ignore all branches less than or equal to this length
 * @return std::vector of neighbour vertices that start branches that are
 * greater than trimLen vertices in length
 */
template <class BidirectionalGraph>
static inline std::vector<typename boost::graph_traits<BidirectionalGraph>::vertex_descriptor>
trueBranches(const typename boost::graph_traits<BidirectionalGraph>::vertex_descriptor& u,
	Direction dir, const BidirectionalGraph& g, unsigned trimLen=0)
{
	typedef BidirectionalGraph G;
	typedef boost::graph_traits<G> graph_traits;
	typedef typename graph_traits::vertex_descriptor V;

	typename graph_traits::out_edge_iterator oei, oei_end;
	typename graph_traits::in_edge_iterator iei, iei_end;

	std::vector<V> branchRoots;

	if (dir == FORWARD) {
		for (boost::tie(oei, oei_end) = out_edges(u, g);
				oei != oei_end; ++oei) {
			const V& v = target(*oei, g);
			if (lookAhead(v, dir, trimLen, g))
				branchRoots.push_back(v);
		}
	} else {
		assert(dir == REVERSE);
		for (boost::tie(iei, iei_end) = in_edges(u, g);
			iei != iei_end; ++iei) {
			const V& v = source(*iei, g);
			if (lookAhead(v, dir, trimLen, g)) {
				branchRoots.push_back(v);
			}
		}
	}

	return branchRoots;
}

/**
 * Return the outgoing neighbour of v if it is unique.
 * Neighbour vertices from branches less than or equal
 * to trimLen are ignored.
 *
 * @param v the subject vertex
 * @param g the graph
 * @param trimLen ignore branches shorter than or equal
 * to this length
 * @param successor if successor is non-NULL
 * and the outgoing neighbour of v is unique, successor will
 * be set to point to the unique outgoing neighbour.
 * Otherwise the value of successor will become undefined.
 * @return SingleExtensionResult (SE_DEAD_END, SE_BRANCHING_POINT,
 * SE_EXTENDED)
 */
template <class Graph>
static inline SingleExtensionResult getSuccessor(
	const typename boost::graph_traits<Graph>::vertex_descriptor& v,
	const Graph& g, unsigned trimLen,
	typename boost::graph_traits<Graph>::vertex_descriptor* successor = NULL)
{
	typedef typename boost::graph_traits<Graph> graph_traits;
	typedef typename graph_traits::vertex_descriptor V;

	typename graph_traits::out_edge_iterator oei, oei_end;
	boost::tie(oei, oei_end) = out_edges(v, g);

	/* 0 neighbours */
	if (oei == oei_end)
		return SE_DEAD_END;

	/* 1 neighbour */
	const V& neighbour1 = target(*oei, g);
	++oei;
	if (oei == oei_end) {
		if (successor != NULL)
			*successor = neighbour1;
		return SE_EXTENDED;
	}

	/* test for 2 or more branches of sufficient length */
	unsigned trueBranches = 0;
	if (lookAhead(neighbour1, FORWARD, trimLen, g))
		trueBranches++;
	for (; oei != oei_end; ++oei) {
		const V& neighbour = target(*oei, g);
		if (lookAhead(neighbour, FORWARD, trimLen, g)) {
			trueBranches++;
			if (trueBranches >= 2)
				return SE_BRANCHING_POINT;
			if (successor != NULL)
				*successor = neighbour;
		}
	}

	if (trueBranches == 0)
		return SE_DEAD_END;
	else if (trueBranches == 1)
		return SE_EXTENDED;
	else
		return SE_BRANCHING_POINT;
}

/**
 * Return the incoming neighbour of v if it is unique.
 * Neighbour vertices from branches less than or equal
 * to trimLen are ignored.
 *
 * @param v the subject vertex
 * @param g the graph
 * @param trimLen ignore branches shorter than or equal
 * to this length
 * @param predecessor if predecessor is non-NULL
 * and the incoming neighbour of v is unique, predecessor will
 * be set to point to the unique incoming neighbour.
 * Otherwise the value of predecessor will become undefined.
 * @return SingleExtensionResult (SE_DEAD_END, SE_BRANCHING_POINT,
 * SE_EXTENDED)
 */
template <class Graph>
static inline SingleExtensionResult getPredecessor(
	const typename boost::graph_traits<Graph>::vertex_descriptor& v,
	const Graph& g, unsigned trimLen,
	typename boost::graph_traits<Graph>::vertex_descriptor* predecessor = NULL)
{
	typedef typename boost::graph_traits<Graph> graph_traits;
	typedef typename graph_traits::vertex_descriptor V;

	typename graph_traits::in_edge_iterator iei, iei_end;
	boost::tie(iei, iei_end) = in_edges(v, g);

	/* 0 neighbours */
	if (iei == iei_end)
		return SE_DEAD_END;

	/* 1 neighbour */
	const V& neighbour1 = source(*iei, g);
	++iei;
	if (iei == iei_end) {
		if (predecessor != NULL)
			*predecessor = neighbour1;
		return SE_EXTENDED;
	}

	/* test for 2 or more branches of sufficient length */
	unsigned trueBranches = 0;
	if (lookAhead(neighbour1, REVERSE, trimLen, g))
		trueBranches++;
	for (; iei != iei_end; ++iei) {
		const V& neighbour = source(*iei, g);
		if (lookAhead(neighbour, REVERSE, trimLen, g)) {
			trueBranches++;
			if (trueBranches >= 2)
				return SE_BRANCHING_POINT;
			if (predecessor != NULL)
				*predecessor = neighbour;
		}
	}
	if (trueBranches == 0)
		return SE_DEAD_END;
	else if (trueBranches == 1)
		return SE_EXTENDED;
	else
		return SE_BRANCHING_POINT;
}

/**
 * Return the single vertex extension of v if it is unique.
 * Neighbour vertices with branches less than or equal
 * to trimLen are ignored.
 *
 * @param v the subject vertex
 * @param dir direction for extension
 * (FORWARD or REVERSE)
 * @param g the graph
 * @param trimLen ignore branches shorter than or equal
 * to this length
 * @param vNext if the extension of v is unique, this will
 * be set to the unique incoming/outgoing neighbour. Otherwise the
 * value will become undefined.
 * @return SingleExtensionResult (SE_DEAD_END, SE_BRANCHING_POINT,
 * SE_EXTENDED)
 */
template <class Graph>
static inline SingleExtensionResult getSingleVertexExtension(
	const typename boost::graph_traits<Graph>::vertex_descriptor& v,
	Direction dir, const Graph& g, unsigned trimLen,
	typename boost::graph_traits<Graph>::vertex_descriptor& vNext)
{
	typedef typename boost::graph_traits<Graph> graph_traits;
	typedef typename graph_traits::vertex_descriptor V;

	SingleExtensionResult result;

	/*
	 * Check number of incoming neighbours.
	 * (We can't extend forwards if there are
	 * multiple incoming branches.)
	 */
	if (dir == FORWARD) {
		result = getPredecessor(v, g, trimLen);
	} else {
		assert(dir == REVERSE);
		result = getSuccessor(v, g, trimLen);
	}
	if (result == SE_BRANCHING_POINT)
		return SE_BRANCHING_POINT;

	/* check number of outgoing branches */
	if (dir == FORWARD) {
		result = getSuccessor(v, g, trimLen, &vNext);
	} else {
		assert(dir == REVERSE);
		result = getPredecessor(v, g, trimLen, &vNext);
	}
	return result;
}

/**
 * If the given path has only one possible next/prev vertex in the graph,
 * append/prepend that vertex to the path.
 *
 * @param path the path to extend (a list of vertices)
 * @param dir direction of extension (FORWARD or REVERSE)
 * @param g the graph to use for traversal
 * @param trimLen ignore neighbour vertices with branches
 * shorter than this length [0]
 * @return PathExtensionResult: NO_EXTENSION, HIT_BRANCHING_POINT, or EXTENDED
 */
template <class BidirectionalGraph>
static inline SingleExtensionResult extendPathBySingleVertex(
	Path<typename boost::graph_traits<BidirectionalGraph>::vertex_descriptor>& path,
	Direction dir, const BidirectionalGraph& g, unsigned trimLen = 0)
{
	typedef BidirectionalGraph G;
	typedef boost::graph_traits<G> graph_traits;
	typedef typename graph_traits::vertex_descriptor V;

	V& v = (dir == FORWARD) ? path.back() : path.front();
	SingleExtensionResult result;
	V vNext;
	result = getSingleVertexExtension(v, dir, g, trimLen, vNext);
	if (result == SE_EXTENDED) {
		if (dir == FORWARD) {
			path.push_back(vNext);
		} else {
			assert(dir == REVERSE);
			path.push_front(vNext);
		}
	}
	return result;
}

/**
 * Extend a path up to the next branching point in the graph.
 *
 * @param path path to extend (modified by this function)
 * @param dir direction to extend path (FORWARD or REVERSE)
 * @param g graph in which to perform the extension
 * @param visited set of previously visited vertices (used
 * to detect cycles in the de Bruijn graph)
 * @param trimLen ignore branches less than this length when
 * detecting branch points [0]
 * @return PathExtensionResult: NO_EXTENSION, HIT_BRANCHING_POINT,
 * or EXTENDED.
 */
template <class BidirectionalGraph>
static inline PathExtensionResult extendPath(
	Path<typename boost::graph_traits<BidirectionalGraph>::vertex_descriptor>& path,
	Direction dir, const BidirectionalGraph& g,
	unordered_set<typename boost::graph_traits<BidirectionalGraph>::vertex_descriptor>& visited,
	unsigned trimLen = 0, unsigned maxLen = NO_LIMIT)
{
	typedef BidirectionalGraph G;
	typedef boost::graph_traits<G> graph_traits;
	typedef typename graph_traits::vertex_descriptor V;
	typename graph_traits::out_edge_iterator oei, oei_end;
	typename graph_traits::in_edge_iterator iei, iei_end;

	assert(path.size() > 0);
	size_t origPathLen = path.size();

	if (path.size() != NO_LIMIT && path.size() >= maxLen)
		return LENGTH_LIMIT;

	SingleExtensionResult result = SE_EXTENDED;
	bool detectedCycle = false;

	while (result == SE_EXTENDED && !detectedCycle &&
		path.size() < maxLen)
	{
		result = extendPathBySingleVertex(path, dir, g, trimLen);
		if (result == SE_EXTENDED) {
			std::pair<typename unordered_set<V>::iterator,bool> inserted;
			if (dir == FORWARD) {
				inserted = visited.insert(path.back());
			} else {
				assert(dir == REVERSE);
				inserted = visited.insert(path.front());
			}
			if (!inserted.second)
				detectedCycle = true;
		}
	}

	/** the last kmer we added is a repeat, so remove it */
	if (detectedCycle) {
		if (dir == FORWARD) {
			path.pop_back();
		} else {
			assert(dir == REVERSE);
			path.pop_front();
		}
	}

	if (path.size() > origPathLen) {
		if (detectedCycle) {
			return EXTENDED_TO_CYCLE;
		} else if (result == SE_DEAD_END) {
			return EXTENDED_TO_DEAD_END;
		} else if (result == SE_BRANCHING_POINT) {
			return EXTENDED_TO_BRANCHING_POINT;
		} else {
			assert(result == SE_EXTENDED &&
				path.size() == maxLen);
			return EXTENDED_TO_LENGTH_LIMIT;
		}
	} else {
		assert(path.size() == origPathLen);
		if (detectedCycle) {
			return CYCLE;
		} else if (result == SE_DEAD_END) {
			return DEAD_END;
		} else if (result == SE_BRANCHING_POINT) {
			return BRANCHING_POINT;
		} else {
			assert(origPathLen >= maxLen);
			return LENGTH_LIMIT;
		}
	}
}

/**
 * Extend a path up to the next branching point in the graph.
 *
 * @param path path to extend (modified by this function)
 * @param dir direction to extend path (FORWARD or REVERSE)
 * @param g graph in which to perform the extension
 * @param trimLen ignore branches less than this length when
 * detecting branch points [0]
 * @return PathExtensionResult: NO_EXTENSION, HIT_BRANCHING_POINT,
 * or EXTENDED.
 */
template <class BidirectionalGraph>
PathExtensionResult extendPath(
	Path<typename boost::graph_traits<BidirectionalGraph>::vertex_descriptor>& path,
	Direction dir, const BidirectionalGraph& g,
	unsigned trimLen = 0, unsigned maxLen = NO_LIMIT)
{
	typedef typename boost::graph_traits<BidirectionalGraph>::vertex_descriptor V;

	/* track visited nodes to avoid infinite traversal of cycles */
	unordered_set<V> visited;
	visited.insert(path.begin(), path.end());

	return extendPath(path, dir, g, visited, trimLen, maxLen);
}

#endif
