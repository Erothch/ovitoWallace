#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <algorithm>
#include "initialize_data.hpp"

#include <array>
using namespace std;


static void make_facets_clockwise(int num_facets, int8_t (*facets)[3], const double (*points)[3])
{
	double plane_normal[3];
	double origin[3] = {0, 0, 0};

	for (int i = 0;i<num_facets;i++)
		add_facet(points, facets[i][0], facets[i][1], facets[i][2], facets[i], plane_normal, origin);
}

static int initialize_graphs(const refdata_t* s, int8_t* colours)
{
	for (int i = 0;i<s->num_graphs;i++)
	{
		std::array<int8_t, 2 * PTM_MAX_EDGES> code;

		int8_t degree[PTM_MAX_NBRS];
		int _max_degree = graph_degree(s->num_facets, s->graphs[i].facets, s->num_nbrs, degree);
		assert(_max_degree <= s->max_degree);

		make_facets_clockwise(s->num_facets, s->graphs[i].facets, &s->points[1]);
		int ret = canonical_form_coloured(s->num_facets, s->graphs[i].facets, s->num_nbrs, degree, colours, s->graphs[i].canonical_labelling, (int8_t*)&code[0], &s->graphs[i].hash);
		if (ret != 0)
			return ret;		
	}

	return PTM_NO_ERROR;
}

bool ptm_initialized = false;
int ptm_initialize_global()
{
	if (ptm_initialized)
		return PTM_NO_ERROR;

	int8_t colours[PTM_MAX_POINTS] = {0};
	int8_t dcolours[PTM_MAX_POINTS] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	int ret = initialize_graphs(&structure_sc, colours);
	ret |= initialize_graphs(&structure_fcc, colours);
	ret |= initialize_graphs(&structure_hcp, colours);
	ret |= initialize_graphs(&structure_ico, colours);
	ret |= initialize_graphs(&structure_bcc, colours);

	ret |= initialize_graphs(&structure_dcub, dcolours);
	ret |= initialize_graphs(&structure_dhex, dcolours);

	if (ret == PTM_NO_ERROR)
		ptm_initialized = true;

	return ret;
}

ptm_local_handle_t ptm_initialize_local()
{
	assert(ptm_initialized);
	return (ptm_local_handle_t)voronoi_initialize_local();
}

void ptm_uninitialize_local(ptm_local_handle_t ptr)
{
	voronoi_uninitialize_local(ptr);
}

