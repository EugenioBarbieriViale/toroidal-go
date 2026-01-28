#include <stdio.h>
#include <raylib.h>

const int FPS = 30;
const int W =  800;
const int H =  600;

const int MOUSE_SPEED = 2;

int main() {
    InitWindow(W, H, "Toroidal Go");
    SetTargetFPS(FPS);

    Camera camera = {0};
    camera.position = (Vector3){10.f, 10.f, 10.f};
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f; 
    camera.projection = CAMERA_PERSPECTIVE;            

    Ray ray = {0};

    Mesh torus = GenMeshTorus(0.6f, 12, 16, 32);
    Model model = LoadModelFromMesh(torus);

    Texture2D texture = LoadTexture("imgs/board.png");
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    BoundingBox box = GetMeshBoundingBox(model.meshes[0]);

    // DisableCursor();

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_THIRD_PERSON);

        // if (IsKeyDown(KEY_UP)) camera.position.z += 1;
        // if (IsKeyDown(KEY_DOWN)) camera.position.z -= 1;
        // if (IsKeyDown(KEY_RIGHT)) camera.position.x -= 1;
        // if (IsKeyDown(KEY_LEFT)) camera.position.x += 1;
        camera.position.z -= MOUSE_SPEED * GetMouseWheelMove();

        RayCollision collision = {0};
        collision.distance = 100;
        collision.hit = false;

        ray = GetScreenToWorldRay(GetMousePosition(), camera);
        RayCollision boxHitInfo = GetRayCollisionBox(ray, box);

        if ((boxHitInfo.hit) && (boxHitInfo.distance < collision.distance)) {
            collision = boxHitInfo;

            RayCollision meshHitInfo = { 0 };
            for (int m = 0; m < model.meshCount; m++) {
                meshHitInfo = GetRayCollisionMesh(ray, model.meshes[m], model.transform);

                if (meshHitInfo.hit) {
                    if ((!collision.hit) || (collision.distance > meshHitInfo.distance)) collision = meshHitInfo;
                    break;
                }
            }

            if (meshHitInfo.hit) {
                collision = meshHitInfo;
                printf("hello\n");
            }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                if (boxHitInfo.hit) DrawBoundingBox(box, LIME);

                DrawModel(model, (Vector3){ 0.5f, 0.0f, 0.0f }, 1, BEIGE);
                DrawGrid(40, 1.0f);

            EndMode3D();
        EndDrawing();
    }
     
    UnloadModel(model);
    CloseWindow();

    return 0;
}
