#ifndef GL_CANVAS_HEIGHTMAP_HIT_TEST_H
#define GL_CANVAS_HEIGHTMAP_HIT_TEST_H

class MyGLCanvas;
struct SpriteInstance;

// Queries heightmap floor/collision information for object placement and rendering.
class GLCanvasHeightmapHitTest {
public:
    // Creates a read-only hit-test adapter over canvas state.
    explicit GLCanvasHeightmapHitTest(const MyGLCanvas& canvas);

    // Returns the highest floor under the supplied room-space rectangle.
    float FloorUnderRect(float min_x, float min_y, float max_x, float max_y) const;
    // Returns the floor height under a room-space point.
    float FloorUnderPoint(float x, float y) const;
    // Tests whether heightmap geometry occludes a sprite shadow rectangle.
    bool ShadowOccludedByHeightmap(float min_x, float min_y, float max_x, float max_y, float z) const;
    // Tests whether the entity hitbox intersects blocking heightmap geometry.
    bool EntityCollidesWithHeightmap(const SpriteInstance& inst) const;
    // Returns the floor height under a square entity hitbox.
    float FloorUnderHitbox(float center_x, float center_y, float half_base) const;

private:
    // Canvas whose room/preview map state is queried.
    const MyGLCanvas& m_canvas;
};

#endif  // GL_CANVAS_HEIGHTMAP_HIT_TEST_H
