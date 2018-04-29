#include "stdafx.h"

#include "../xrRender/du_cone.h"
#include "../xrRenderDX10/dx10BufferUtils.h"

void CRenderTarget::accum_spot_geom_create	()
{
	// vertices
	{
		u32		vCount		= DU_CONE_NUMVERTEX;
		u32		vSize		= 3*4;

		R_CHK(dx10BufferUtils::CreateVertexBuffer	( &g_accum_spot_vb, du_cone_vertices,vCount*vSize));
		HW.stats_manager.increment_stats_vb			( g_accum_spot_vb );
	}

	// Indices
	{
		u32		iCount		= DU_CONE_NUMFACES*3;

		R_CHK( dx10BufferUtils::CreateIndexBuffer	( &g_accum_spot_ib, du_cone_faces, iCount*2 ) );
		HW.stats_manager.increment_stats_ib			( g_accum_spot_ib );
	}
}

void CRenderTarget::accum_spot_geom_destroy()
{
#ifdef DEBUG
	_SHOW_REF	("g_accum_spot_ib",g_accum_spot_ib);
#endif // DEBUG
	HW.stats_manager.decrement_stats_ib	(g_accum_spot_ib);
	_RELEASE	(g_accum_spot_ib);
#ifdef DEBUG
	_SHOW_REF	("g_accum_spot_vb",g_accum_spot_vb);
#endif // DEBUG
	HW.stats_manager.decrement_stats_vb	(g_accum_spot_vb);
	_RELEASE	(g_accum_spot_vb);
}

struct Slice
{
	Fvector	m_Vert[4];
};

void CRenderTarget::accum_volumetric_geom_create()
{
	// vertices
	{
		//	VOLUMETRIC_SLICES quads
		static const u32		vCount		= VOLUMETRIC_SLICES*4;
		u32		vSize		= 3*4;
		
		Slice	pSlice[VOLUMETRIC_SLICES];

		float t=0;
		float dt = 1.0f/(VOLUMETRIC_SLICES-1);
		for ( int i=0; i<VOLUMETRIC_SLICES; ++i)
		{
			pSlice[i].m_Vert[0] = Fvector().set(0,0,t);
			pSlice[i].m_Vert[1] = Fvector().set(0,1,t);
			pSlice[i].m_Vert[2] = Fvector().set(1,0,t);
			pSlice[i].m_Vert[3] = Fvector().set(1,1,t);
			t += dt;
		}

		R_CHK( dx10BufferUtils::CreateVertexBuffer	( &g_accum_volumetric_vb, &pSlice, vCount*vSize) );
		HW.stats_manager.increment_stats_vb			( g_accum_volumetric_vb );
	}

	// Indices
	{
		const u32		iCount		= VOLUMETRIC_SLICES*6;

		BYTE	Datap[iCount*2];

		u16 *pInd = (u16*) Datap;
		for ( u16 i=0; i<VOLUMETRIC_SLICES; ++i, pInd+=6)
		{
			u16 basevert = i*4;
			pInd[0] = basevert;
			pInd[1] = basevert+1;
			pInd[2] = basevert+2;
			pInd[3] = basevert+2;
			pInd[4] = basevert+1;
			pInd[5] = basevert+3;
		}

		R_CHK( dx10BufferUtils::CreateIndexBuffer	( &g_accum_volumetric_ib, &Datap, iCount*2 ) );
		HW.stats_manager.increment_stats_ib			(g_accum_volumetric_ib);
	}
}

void CRenderTarget::accum_volumetric_geom_destroy()
{
#ifdef DEBUG
	_SHOW_REF	("g_accum_volumetric_ib",g_accum_volumetric_ib);
#endif // DEBUG

	HW.stats_manager.decrement_stats_ib(g_accum_volumetric_ib);
	_RELEASE	(g_accum_volumetric_ib);
#ifdef DEBUG
	_SHOW_REF	("g_accum_volumetric_vb",g_accum_volumetric_vb);
#endif // DEBUG

	HW.stats_manager.decrement_stats_vb(g_accum_volumetric_vb);
	_RELEASE	(g_accum_volumetric_vb);
}