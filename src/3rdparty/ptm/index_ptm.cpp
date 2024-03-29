#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <algorithm>
#include "convex_hull_incremental.hpp"
#include "graph_data.hpp"
#include "deformation_gradient.hpp"
#include "alloy_types.hpp"
#include "neighbour_ordering.hpp"
#include "normalize_vertices.hpp"
#include "qcprot/quat.hpp"
#include "qcprot/polar.hpp"
#include "initialize_data.hpp"
#include "structure_matcher.hpp"
#include "ptm_functions.h"
#include "ptm_constants.h"


//todo: verify that c == norm(template[1])
static double calculate_interatomic_distance(int type, double scale)
{
	assert(type >= 1 && type <= 7);
	double c[8] = {0, 1, 1, (7. - 3.5 * sqrt(3)), 1, 1, sqrt(3) * 4. / (6 * sqrt(2) + sqrt(3)), sqrt(3) * 4. / (6 * sqrt(2) + sqrt(3))};
	return c[type] / scale;
}

static double calculate_lattice_constant(int type, double interatomic_distance)
{
	assert(type >= 1 && type <= 7);
	double c[8] = {0, 2 / sqrt(2), 2 / sqrt(2), 2. / sqrt(3), 2 / sqrt(2), 1, 4 / sqrt(3), 4 / sqrt(3)};
	return c[type] * interatomic_distance;
}

static int rotate_into_fundamental_zone(int type, double* q)
{
	if (type == PTM_MATCH_SC)	return rotate_quaternion_into_cubic_fundamental_zone(q);
	if (type == PTM_MATCH_FCC)	return rotate_quaternion_into_cubic_fundamental_zone(q);
	if (type == PTM_MATCH_BCC)	return rotate_quaternion_into_cubic_fundamental_zone(q);
	if (type == PTM_MATCH_ICO)	return rotate_quaternion_into_icosahedral_fundamental_zone(q);
	if (type == PTM_MATCH_HCP)	return rotate_quaternion_into_hcp_fundamental_zone(q);
	if (type == PTM_MATCH_DCUB)	return rotate_quaternion_into_diamond_cubic_fundamental_zone(q);
	if (type == PTM_MATCH_DHEX)	return rotate_quaternion_into_diamond_hexagonal_fundamental_zone(q);
	return -1;
}

static void order_points(ptm_local_handle_t local_handle, int num_points, double (*unpermuted_points)[3], int32_t* unpermuted_numbers, bool topological_ordering,
			int8_t* ordering, double (*points)[3], int32_t* numbers)
{
	if (topological_ordering)
	{
		double normalized_points[PTM_MAX_INPUT_POINTS][3];
		normalize_vertices(num_points, unpermuted_points, normalized_points);
		int ret = calculate_neighbour_ordering((void*)local_handle, num_points, (const double (*)[3])normalized_points, ordering);
		if (ret != 0)
			topological_ordering = false;
	}

	if (!topological_ordering)
		for (int i=0;i<num_points;i++)
			ordering[i] = i;

	for (int i=0;i<num_points;i++)
	{
		memcpy(points[i], &unpermuted_points[ordering[i]], 3 * sizeof(double));

		if (unpermuted_numbers != NULL)
			numbers[i] = unpermuted_numbers[ordering[i]];
	}
}

static void output_data(result_t* res, int num_points, int32_t* unpermuted_numbers, double (*points)[3], int32_t* numbers, int8_t* ordering,
			int32_t* p_type, int32_t* p_alloy_type, double* p_scale, double* p_rmsd, double* q, double* F, double* F_res,
			double* U, double* P, int8_t* mapping, double* p_interatomic_distance, double* p_lattice_constant)
{
	*p_type = PTM_MATCH_NONE;
	if (p_alloy_type != NULL)
		*p_alloy_type = PTM_ALLOY_NONE;

	if (mapping != NULL)
		memset(mapping, -1, num_points * sizeof(int8_t));

	const refdata_t* ref = res->ref_struct;
	if (ref == NULL)
		return;

	*p_type = ref->type;
	if (p_alloy_type != NULL && unpermuted_numbers != NULL)
		*p_alloy_type = find_alloy_type(ref, res->mapping, numbers);

	int bi = rotate_into_fundamental_zone(ref->type, res->q);
	int8_t temp[PTM_MAX_POINTS];
	for (int i=0;i<ref->num_nbrs+1;i++)
		temp[ref->mapping[bi][i]] = res->mapping[i];

	memcpy(res->mapping, temp, (ref->num_nbrs+1) * sizeof(int8_t));

	if (F != NULL && F_res != NULL)
	{
		double scaled_points[PTM_MAX_INPUT_POINTS][3];

		subtract_barycentre(ref->num_nbrs + 1, points, scaled_points);
		for (int i = 0;i<ref->num_nbrs + 1;i++)
		{
			scaled_points[i][0] *= res->scale;
			scaled_points[i][1] *= res->scale;
			scaled_points[i][2] *= res->scale;
		}
		calculate_deformation_gradient(ref->num_nbrs + 1, ref->points, res->mapping, scaled_points, ref->penrose, F, F_res);

		if (P != NULL && U != NULL)
			polar_decomposition_3x3(F, false, U, P);
	}

	if (mapping != NULL)
		for (int i=0;i<ref->num_nbrs + 1;i++)
			mapping[i] = ordering[res->mapping[i]];

	double interatomic_distance = calculate_interatomic_distance(ref->type, res->scale);
	double lattice_constant = calculate_lattice_constant(ref->type, interatomic_distance);

	if (p_interatomic_distance != NULL)
		*p_interatomic_distance = interatomic_distance;

	if (p_lattice_constant != NULL)
		*p_lattice_constant = lattice_constant;

	*p_rmsd = res->rmsd;
	*p_scale = res->scale;
	memcpy(q, res->q, 4 * sizeof(double));
}


extern bool ptm_initialized;

int ptm_index(	ptm_local_handle_t local_handle, int32_t flags,
		int num_points, double (*unpermuted_points)[3], int32_t* unpermuted_numbers, bool topological_ordering,
		int32_t* p_type, int32_t* p_alloy_type, double* p_scale, double* p_rmsd, double* q, double* F, double* F_res,
		double* U, double* P, int8_t* mapping, double* p_interatomic_distance, double* p_lattice_constant)
{
	assert(ptm_initialized);
	assert(num_points <= PTM_MAX_INPUT_POINTS);

	if (flags & PTM_CHECK_SC)
		assert(num_points >= PTM_NUM_POINTS_SC);

	if (flags & PTM_CHECK_BCC)
		assert(num_points >= PTM_NUM_POINTS_BCC);

	if (flags & (PTM_CHECK_FCC | PTM_CHECK_HCP | PTM_CHECK_ICO))
		assert(num_points >= PTM_NUM_POINTS_FCC);

	if (flags & (PTM_CHECK_DCUB | PTM_CHECK_DHEX))
		assert(num_points >= PTM_NUM_POINTS_DCUB);

	int ret = 0;
	result_t res;
	res.ref_struct = NULL;
	res.rmsd = INFINITY;

	int8_t ordering[PTM_MAX_INPUT_POINTS];
	double points[PTM_MAX_POINTS][3];
	int32_t numbers[PTM_MAX_POINTS];

	int8_t dordering[PTM_MAX_INPUT_POINTS];
	double dpoints[PTM_MAX_POINTS][3];
	int32_t dnumbers[PTM_MAX_POINTS];

	convexhull_t ch;
	double ch_points[PTM_MAX_INPUT_POINTS][3];

	if (flags & (PTM_CHECK_SC | PTM_CHECK_FCC | PTM_CHECK_HCP | PTM_CHECK_ICO | PTM_CHECK_BCC))
	{
		int num_lpoints = std::min(std::min(PTM_MAX_POINTS, 20), num_points);
		order_points(local_handle, num_lpoints, unpermuted_points, unpermuted_numbers, topological_ordering, ordering, points, numbers);
		normalize_vertices(num_lpoints, points, ch_points);
		ch.ok = false;

		if (flags & PTM_CHECK_SC)
			ret = match_general(&structure_sc, ch_points, points, &ch, &res);

		if (flags & (PTM_CHECK_FCC | PTM_CHECK_HCP | PTM_CHECK_ICO))
			ret = match_fcc_hcp_ico(ch_points, points, flags, &ch, &res);

		if (flags & PTM_CHECK_BCC)
			ret = match_general(&structure_bcc, ch_points, points, &ch, &res);
	}

	if (flags & (PTM_CHECK_DCUB | PTM_CHECK_DHEX))
	{
		ret = calculate_diamond_neighbour_ordering(num_points, unpermuted_points, unpermuted_numbers, dordering, dpoints, dnumbers);
		if (ret == 0)
		{
			normalize_vertices(PTM_NUM_NBRS_DCUB + 1, dpoints, ch_points);
			ch.ok = false;

			ret = match_dcub_dhex(ch_points, dpoints, flags, &ch, &res);
		}
	}

	if (res.ref_struct != NULL && (res.ref_struct->type == PTM_MATCH_DCUB || res.ref_struct->type == PTM_MATCH_DHEX))
	{
		output_data(	&res, num_points, unpermuted_numbers, dpoints, dnumbers, dordering,
				p_type, p_alloy_type, p_scale, p_rmsd, q, F, F_res,
				U, P, mapping, p_interatomic_distance, p_lattice_constant);
	}
	else
	{
		output_data(	&res, num_points, unpermuted_numbers, points, numbers, ordering,
				p_type, p_alloy_type, p_scale, p_rmsd, q, F, F_res,
				U, P, mapping, p_interatomic_distance, p_lattice_constant);
	}

	return PTM_NO_ERROR;
}

