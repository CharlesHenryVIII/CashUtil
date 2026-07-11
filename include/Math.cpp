#include "Math.h"

u32 PCG_Random(u64 state)
{
    u32 result = static_cast<u32>((state ^ (state >> 22)) >> (22 + (state >> 61)));
    return result;
}


#if 1

float Bilinear(float p00, float p10, float p01, float p11, float x, float y)
{
   float p0 = Lerp(y, p00, p01);
   float p1 = Lerp(y, p10, p11);
   return Lerp(x, p0, p1);
}
#else
[[nodiscard]] float Bilinear(Vec2 p, Rect loc, float bl, float br, float tl, float tr)
{
    float denominator = ((loc.topRight.x - loc.botLeft.x) * (loc.topRight.y - loc.botLeft.y));

    float xLeftNum  = (loc.topRight.x - p.x);
    float xRightNum = (p.x            - loc.botLeft.x);
    float yBotNum   = (loc.topRight.y - p.y);
    float yTopNum   = (p.y            - loc.botLeft.y);

    float c1 = bl * ((xLeftNum  * yBotNum) / denominator);
    float c2 = br * ((xRightNum * yBotNum) / denominator);
    float c3 = tl * ((xLeftNum  * yTopNum) / denominator);
    float c4 = tr * ((xRightNum * yTopNum) / denominator);

    return c1 + c2 + c3 + c4;
}
#endif

[[nodiscard]] float Cubic( Vec4 v, float x )
{
    float a = 0.5f * (v.w - v.x) + 1.5f * (v.y - v.z);
    float b = 0.5f * (v.x + v.z) - v.y - a;
    float c = 0.5f * (v.z - v.x);
    float d = v.y;

    return d + x * (c + x * (b + x * a));
}

[[nodiscard]] float Bicubic(Mat4 p, Vec2 pos)
{
    Vec4 a;
    a.e[0] = Cubic(p.col[0], pos.y);
    a.e[1] = Cubic(p.col[1], pos.y);
    a.e[2] = Cubic(p.col[2], pos.y);
    a.e[3] = Cubic(p.col[3], pos.y);
    return Cubic(a, pos.x);
}


static void matd_mul(float out[4][4], float src1[4][4], float src2[4][4])
{
   int i,j,k;
   for (j=0; j < 4; ++j)
      for (i=0; i < 4; ++i) {
         float t=0;
         for (k=0; k < 4; ++k)
            t += src1[k][i] * src2[j][k];
         out[i][j] = t;
      }
}

bool SphereVsFrustum(const Sphere& sphere, const Frustum& frustum, bool ignore_near_z)
{
    FAIL;
    bool result = false;
    u32 num_planes = ignore_near_z ? 5 : 6;
    for(u32 i = 0; i < num_planes; i++) {
        float distance = DotProduct(frustum.e[i].xyz, sphere.center);

        if (distance < -sphere.radius)
            return false;
        else if (distance < sphere.radius)
            result = true;
    }

    return result;
}

Frustum ComputeFrustum(const Mat4& in)
{
    Frustum result = {};
    ASSERT(false); //Is matd_mul() actually used?
    float out[4][4];
    float in1[4][4];
    float in2[4][4];

    matd_mul(out, in1, in2);
    Mat4 mvProj = in;
    gb_mat4_transpose<float>(mvProj);

    for (i32 i = 0; i < 4; ++i)
    {
        (&result.e[0].x)[i] = mvProj.col[3].e[i] + mvProj.col[0].e[i];
        (&result.e[1].x)[i] = mvProj.col[3].e[i] - mvProj.col[0].e[i];
        (&result.e[2].x)[i] = mvProj.col[3].e[i] + mvProj.col[1].e[i];
        (&result.e[3].x)[i] = mvProj.col[3].e[i] - mvProj.col[1].e[i];
        (&result.e[4].x)[i] = mvProj.col[3].e[i] + mvProj.col[2].e[i];
        (&result.e[5].x)[i] = mvProj.col[3].e[i] - mvProj.col[2].e[i];
    }
    return result;
}

i32 TestPlane(const Vec4 *p, float x0, float y0, float z0, float x1, float y1, float z1)
{
   // return false if the box is entirely behind the plane
   float d = 0;
   ASSERT(x0 <= x1 && y0 <= y1 && z0 <= z1);
   if (p->x > 0) d += x1 * p->x; else d += x0 * p->x;
   if (p->y > 0) d += y1 * p->y; else d += y0 * p->y;
   if (p->z > 0) d += z1 * p->z; else d += z0 * p->z;
   return d + p->w >= 0;
}

bool IsBoxInFrustum(const Frustum& f, float *bmin, float *bmax)
{
   i32 i;
   for (i=0; i < 5; ++i)
      if (!TestPlane(&f.e[i], bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]))
         return 0;
   return 1;
}

i32 ManhattanDistance(Vec3I a, Vec3I b)
{
    return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
}



void Swap(void* a, void* b, const i32 size)
{

    u8* c = (u8*)a;
    u8* d = (u8*)b;
    for (i32 i = 0; i < size; i++)
    {

        u8 temp = c[i];
        c[i] = d[i];
        d[i] = temp;
    }
}

int Partition(u8* array, const i32 itemSize, i32 iBegin, i32 iEnd, i32 (*compare)(const void*, const void*))
{
    ASSERT(array != nullptr);
    u8* pivot = &array[iEnd * itemSize];
    ASSERT(pivot != nullptr);
    i32 lowOffset = iBegin;

    for (i32 i = iBegin; i < iEnd; i++)
    {
        if (compare(&array[i * itemSize], pivot) > 0)
        {
            Swap(&array[lowOffset * itemSize], &array[i * itemSize], itemSize);
            lowOffset++;
        }
    }

    Swap(&array[lowOffset * itemSize], &array[iEnd * itemSize], itemSize);
    return lowOffset;
}


void QuickSortInternal(u8* array, const i32 itemSize, i32 iBegin, i32 iEnd, i32 (*compare)(const void*, const void*))
{
    if (iBegin < iEnd)
    {
        i32 pivotIndex = Partition(array, itemSize, iBegin, iEnd, compare);
        QuickSortInternal(array, itemSize, iBegin, pivotIndex - 1, compare); //Low Sort
        QuickSortInternal(array, itemSize, pivotIndex + 1, iEnd, compare); //High Sort
    }
}

void QuickSort(u8* data, const i32 length, const i32 itemSize, i32 (*compare)(const void* a, const void* b))
{
    QuickSortInternal(data, itemSize, 0, length - 1, compare);
}
