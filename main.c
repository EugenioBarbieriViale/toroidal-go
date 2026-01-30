#include <raylib.h>
#include <raymath.h>

#define S(a, c) 4*PI*PI*(a)*(c)


int main() {
    const int FPS = 30;
    const int W =  800;
    const int H =  600;

    const int MOUSE_SPEED = 4;

    const double A = 0.6f;
    const double C = 22.f;
    const double R = pow(C, 0.5);

    const double OFFSET = 0.5;


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

    Model torus = LoadModelFromMesh(GenMeshTorus(A, C, 16, 32));
    Model black = LoadModelFromMesh(GenMeshSphere(1, 32, 32));
    Model white = LoadModelFromMesh(GenMeshSphere(1, 32, 32));

    Texture2D texture = LoadTexture("imgs/board.png");
    torus.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    Vector3 stones[361];

    int count = 0;
    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) camera.position.y += 1;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) camera.position.y -= 1;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) camera.position.x += 1;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) camera.position.x -= 1;
        camera.position.z -= MOUSE_SPEED * GetMouseWheelMove();

        ray = GetScreenToWorldRay(GetMousePosition(), camera);
        RayCollision collision = GetRayCollisionMesh(ray, torus.meshes[0], torus.transform);

        Vector3 pos = {-R + OFFSET, 0, 0};
        RayCollision point_collision = GetRayCollisionSphere(ray, pos, 0.3);

        if (collision.hit && point_collision.hit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            double len_norm = Vector3Length(collision.normal);
            Vector3 offset_v = Vector3Scale(collision.normal, OFFSET / len_norm);

            stones[count] = Vector3Subtract(collision.point, offset_v);
            count++;
        }

        BeginDrawing();
            // ClearBackground(RAYWHITE);
            ClearBackground(GRAY);

            BeginMode3D(camera);

                for (int i=0; i<count; i++) {
                    if (i % 2 == 0)
                        DrawModel(black, stones[i], 1, BLACK);
                    else
                        DrawModel(white, stones[i], 1, WHITE);
                }

                DrawModel(torus, (Vector3){ 0.0f, 0.0f, 0.0f }, 1, BEIGE);

            EndMode3D();
        EndDrawing();
    }
     
    UnloadModel(torus);
    CloseWindow();

    return 0;
}
