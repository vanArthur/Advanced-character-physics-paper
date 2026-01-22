#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>

#define WIDTH 1000
#define HEIGHT 1000

// Cloth settings
#define CLOTH_COLS 40
#define CLOTH_ROWS 30
#define SPACING 20 // Distance between particles
#define START_X 100
#define START_Y 100

#define PARTICLE_RADIUS 3
#define PARTICLE_COLOR GetColor(0xFF6F61ff)
#define CONSTRAINT_COLOR RAYWHITE

// Physics settings
#define GRAVITY 0.5f
#define TIME_STEP 0.2f
#define NUM_ITERATIONS 5 // Increase iterations for stiffer cloth

typedef struct {
  Vector3 position;
  Vector3 prev_position;
  Vector3 acceleration;
  bool is_pinned;
  Color color;
} Particle;

typedef struct Constraint {
  int p1;
  int p2;
  float rest_length;
} Constraint;

typedef struct {
  Particle *particles;
  Constraint *constraints;
  int particle_count;
  int constraint_count;
} ParticleSystem;

ParticleSystem psystem = {0};

Particle create_particle(float x, float y, float z, Color color, bool pinned) {
  Particle p;
  p.position = (Vector3){x, y, z};
  p.prev_position = (Vector3){x, y, z};
  p.acceleration = (Vector3){0, 0, 0};
  p.color = color;
  p.is_pinned = pinned;
  return p;
}

// check if ray intersects with plane and return intersection point
// source:
// https://lousodrome.net/blog/light/2020/07/03/intersection-of-a-ray-and-a-plane/
Vector3 GetRayPlaneIntersection(Ray ray, Vector3 planePos,
                                Vector3 planeNormal) {
  float denominator = Vector3DotProduct(ray.direction, planeNormal);
  // Check if the ray is parallel to the plane, dotproduct is zero, no
  // intersection
  if (fabs(denominator) > 1e-6) {

    // Numerator: ((PlanePos - RayPos) . PlaneNormal)
    Vector3 vectorToPlane = Vector3Subtract(planePos, ray.position);
    // projected onto plane normal
    float numerator = Vector3DotProduct(vectorToPlane, planeNormal);

    // t (Distance)
    float t = numerator / denominator;

    // final Point: Origin + (Direction * t)
    return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
  }

  // Return zero vector if no intersection
  return (Vector3){0};
}

Constraint create_constraint(int p1, int p2, float rest_length) {
  Constraint c;
  c.p1 = p1;
  c.p2 = p2;
  c.rest_length = rest_length;
  return c;
}

void verlet(ParticleSystem *psystem) {
  for (int i = 0; i < psystem->particle_count; i++) {
    Particle *p = &psystem->particles[i];

    if (p->is_pinned)
      continue;

    Vector3 temp = p->position;

    // Calculate Velocity
    Vector3 velocity = Vector3Subtract(p->position, p->prev_position);
    velocity = Vector3Scale(velocity, 0.99f); // Damping

    // Calculate Acceleration term (a * dt * dt)
    Vector3 accelerationStep =
        Vector3Scale(p->acceleration, TIME_STEP * TIME_STEP);

    // Verlet Integration: next = curr + vel + acc
    Vector3 nextPos =
        Vector3Add(Vector3Add(p->position, velocity), accelerationStep);

    p->position = nextPos;
    p->prev_position = temp;
  }
}

void accumulate_forces(ParticleSystem *psystem) {
  for (int i = 0; i < psystem->particle_count; i++) {
    Particle *p = &psystem->particles[i];
    p->acceleration.y = GRAVITY;

    if (IsKeyDown(KEY_SPACE)) {
      p->acceleration.x = 0.8f; // Wind blowing right
    } else {
      p->acceleration.x = 0.0f;
    }
  }
}

void satisfy_constraints(ParticleSystem *psystem) {
  for (int j = 0; j < NUM_ITERATIONS; j++) {
    for (int i = 0; i < psystem->constraint_count; i++) {
      Constraint *c = &psystem->constraints[i];
      Particle *p1 = &psystem->particles[c->p1];
      Particle *p2 = &psystem->particles[c->p2];

      Vector3 delta = Vector3Subtract(p2->position, p1->position);

      float current_dist = Vector3Length(delta);

      // Avoid division by zero
      if (current_dist == 0.0f)
        continue;

      // Calculate the difference ratio
      // how far we are from rest length vs current length
      float difference = (current_dist - c->rest_length) / current_dist;

      // We want to move each particle half the difference
      Vector3 correction = Vector3Scale(delta, difference * 0.5f);

      if (!p1->is_pinned && !p2->is_pinned) {
        p1->position = Vector3Add(p1->position, correction);
        p2->position = Vector3Subtract(p2->position, correction);
      } else if (p1->is_pinned && !p2->is_pinned) {
        p2->position =
            Vector3Subtract(p2->position, Vector3Scale(correction, 2.0f));
      } else if (!p1->is_pinned && p2->is_pinned) {
        p1->position = Vector3Add(p1->position, Vector3Scale(correction, 2.0f));
      }
    }
  }
}

void time_step(ParticleSystem *psystem) {
  accumulate_forces(psystem);
  verlet(psystem);
  satisfy_constraints(psystem);
}

int main(void) {
  InitWindow(WIDTH, HEIGHT, "Advanced Character Physics");
  SetTargetFPS(120);

  Camera3D camera = {0};
  rlSetClipPlanes(0.1f, 2000.0f);

  Vector3 target = {START_X + (CLOTH_COLS * SPACING) / 2.0f,
                    START_Y + (CLOTH_ROWS * SPACING) / 2.0f, 0.0f};
  camera.target = target;
  camera.position = (Vector3){target.x, target.y, 800.0f};
  camera.up = (Vector3){0.0f, -1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  // Allocate
  int num_particles = CLOTH_COLS * CLOTH_ROWS;
  int num_constraints =
      (CLOTH_COLS - 1) * CLOTH_ROWS + (CLOTH_ROWS - 1) * CLOTH_COLS;
  psystem.particles = malloc(sizeof(Particle) * num_particles);
  psystem.constraints = malloc(sizeof(Constraint) * num_constraints);

  // Init Particles
  for (int y = 0; y < CLOTH_ROWS; y++) {
    for (int x = 0; x < CLOTH_COLS; x++) {
      int index = y * CLOTH_COLS + x;
      float px = START_X + x * SPACING;
      float py = START_Y + y * SPACING;

      bool pin = (y == 0 && (x % 5 == 0 || x == CLOTH_COLS - 1));

      psystem.particles[index] =
          create_particle(px, py, 0, PARTICLE_COLOR, pin);
      psystem.particle_count++;
    }
  }

  // Init Constraints
  for (int y = 0; y < CLOTH_ROWS; y++) {
    for (int x = 0; x < CLOTH_COLS; x++) {
      int current_idx = y * CLOTH_COLS + x;
      if (x < CLOTH_COLS - 1) {
        psystem.constraints[psystem.constraint_count++] =
            create_constraint(current_idx, y * CLOTH_COLS + (x + 1), SPACING);
      }
      if (y < CLOTH_ROWS - 1) {
        psystem.constraints[psystem.constraint_count++] =
            create_constraint(current_idx, (y + 1) * CLOTH_COLS + x, SPACING);
      }
    }
  }

  int dragged_particle_idx = -1;
  float time_counter = 0.0f;

  while (!WindowShouldClose()) {
    SetTargetFPS(120);
    float dt = GetFrameTime();
    time_counter += dt * 0.1f;

    // --- Camera Update ---
    float radius = 1000.0f;
    camera.position.x = target.x + radius * sinf(time_counter);
    camera.position.z = radius * cosf(time_counter);
    camera.position.y = target.y - 300.0f;

    // --- Mouse Interaction ---
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Ray ray = GetMouseRay(GetMousePosition(), camera);
      float min_dist = 100000.0f;
      int closest_idx = -1;

      for (int i = 0; i < psystem.particle_count; i++) {
        RayCollision collision =
            GetRayCollisionSphere(ray, psystem.particles[i].position, 15.0f);
        if (collision.hit && collision.distance < min_dist) {
          min_dist = collision.distance;
          closest_idx = i;
        }
      }
      dragged_particle_idx = closest_idx;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      dragged_particle_idx = -1;
    }

    if (dragged_particle_idx != -1) {
      Ray ray = GetMouseRay(GetMousePosition(), camera);

      // Create a plane at the particle's current Z depth to drag along
      Vector3 planePos = psystem.particles[dragged_particle_idx].position;
      Vector3 planeNormal = {0, 0, 1}; // Dragging on XY plane

      Vector3 hitPoint = GetRayPlaneIntersection(ray, planePos, planeNormal);

      psystem.particles[dragged_particle_idx].position = hitPoint;
      // Reset prev_position to kill velocity
      psystem.particles[dragged_particle_idx].prev_position = hitPoint;
    }

    time_step(&psystem);

    BeginDrawing();
    ClearBackground(GetColor(0x052A4Fff));
    BeginMode3D(camera);

    // Draw Constraints
    for (int i = 0; i < psystem.constraint_count; i++) {
      Constraint c = psystem.constraints[i];
      Vector3 p1 = psystem.particles[c.p1].position;
      Vector3 p2 = psystem.particles[c.p2].position;
      DrawLine3D(p1, p2, Fade(CONSTRAINT_COLOR, 0.4f));
    }

    // Draw Particles
    for (int i = 0; i < psystem.particle_count; i++) {
      Particle *p = &psystem.particles[i];
      if (p->is_pinned)
        DrawCircle3D(p->position, PARTICLE_RADIUS + 2.f, (Vector3){0,1,0}, 0, RED);
      else
        DrawCircle3D(p->position, PARTICLE_RADIUS, (Vector3){0,1,0}, 0, p->color);
    }

    DrawGrid(100, 50.0f);
    EndMode3D();

    DrawText("Space for Wind | Mouse to Drag", 10, 10, 20,
             RAYWHITE);
    DrawFPS(10, 40);
    EndDrawing();
  }

  free(psystem.particles);
  free(psystem.constraints);
  CloseWindow();
  return 0;
}
