/*
 *   Screen capture?
 *
 */

#pragma once

struct IDirectDrawSurface;

void GrpSetCaptureFilename(const char *s);
void PutSurfaceToBmp(IDirectDrawSurface* surf);
