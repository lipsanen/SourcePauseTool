#include "stdafx.h"
#include "graphics.hpp"
#include "convar.h"
#include "..\spt-serverplugin.hpp"
#include "..\..\sptlib-wrapper.hpp"
#include "..\modules.hpp"
#include "..\..\utils\ent_utils.hpp"

ConVar y_spt_drawseams("y_spt_drawseams", "0", FCVAR_CHEAT, "Draws seamshot stuff.\n");

namespace vgui
{
	void DrawSeams(IVDebugOverlay* debugOverlay)
	{
		auto player = GetServerPlayer();

		if (!player)
			return;

		float va[3];
		EngineGetViewAngles(va);
		Vector cameraPosition = clientDLL.GetCameraOrigin();
		QAngle angles(va[0], va[1], va[2]);
		Vector vDirection;
		AngleVectors(angles, &vDirection);

		trace_t tr;
		serverDLL.TraceFirePortal(tr, cameraPosition, vDirection);

		if (tr.fraction < 1.0f)
		{
			trace_t edgeTr;
			const float CORNER_CHECK_LENGTH_SQR = 36.0f * 36.0f;
			utils::FindClosestPlane(tr, edgeTr, CORNER_CHECK_LENGTH_SQR);

			if (edgeTr.fraction < 1.0f)
			{
				const float PORTAL_SHOT_TEST_LEN_SQR = 50.0f * 50.0f;
				Vector test1v = tr.plane.normal + edgeTr.plane.normal * 2;
				Vector test2v = tr.plane.normal * 2 + edgeTr.plane.normal;

				Vector test1o = tr.plane.normal * 0.001;
				Vector test2o = edgeTr.plane.normal * 0.001;

				trace_t test1tr, test2tr;
				serverDLL.TraceFirePortal(test1tr, edgeTr.endpos + test1v + test1o, test1v * -1);
				serverDLL.TraceFirePortal(test2tr, edgeTr.endpos + test2v + test2o, test2v * -1);

				bool test1 = !utils::TraceHit(test1tr, PORTAL_SHOT_TEST_LEN_SQR);
				bool test2 = !utils::TraceHit(test2tr, PORTAL_SHOT_TEST_LEN_SQR);
				bool seamshot = test1 || test2;

				const int uiScale = 10;

				//calculating an edge vector for drawing
				Vector edge = edgeTr.plane.normal.Cross(tr.plane.normal);
				VectorNormalize(edge);
				debugOverlay->AddLineOverlay(edgeTr.endpos - edge * uiScale,
				                             edgeTr.endpos + edge * uiScale,
				                             seamshot ? 0 : 255,
				                             seamshot ? 255 : 0,
				                             0,
				                             true,
				                             0.06);

				// Lines in the direction of the planes
				debugOverlay->AddLineOverlay(edgeTr.endpos,
				                             edgeTr.endpos + edgeTr.plane.normal * uiScale,
				                             test1 ? 0 : 255,
				                             test1 ? 255 : 0,
				                             0,
				                             true,
				                             0.06);
				debugOverlay->AddLineOverlay(edgeTr.endpos,
				                             edgeTr.endpos + tr.plane.normal * uiScale,
				                             test2 ? 0 : 255,
				                             test2 ? 255 : 0,
				                             0,
				                             true,
				                             0.06);

				// Seamshot triangle
				if (seamshot)
				{
					Vector midPoint = edgeTr.endpos + edgeTr.plane.normal * (uiScale / 2.0)
					                  + tr.plane.normal * (uiScale / 2.0);
					debugOverlay->AddLineOverlay(midPoint,
					                             edgeTr.endpos + edgeTr.plane.normal * uiScale,
					                             test1 ? 0 : 255,
					                             test1 ? 255 : 0,
					                             0,
					                             true,
					                             0.06);
					debugOverlay->AddLineOverlay(midPoint,
					                             edgeTr.endpos + tr.plane.normal * uiScale,
					                             test2 ? 0 : 255,
					                             test2 ? 255 : 0,
					                             0,
					                             true,
					                             0.06);
				}
			}
		}
	}

	void DrawLines()
	{
		auto debugOverlay = GetDebugOverlay();

		if (!debugOverlay || !utils::playerEntityAvailable())
			return;

		if (y_spt_drawseams.GetBool())
			DrawSeams(debugOverlay);
	}
} // namespace vgui