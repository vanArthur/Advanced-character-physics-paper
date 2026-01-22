#include <math.h>
#include <raylib.h>
#include <raymath.h>
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

#define GRAVITY 0.5f
#define TIME_STEP 0.2f
#define NUM_ITERATIONS 5 // Increase iterations for stiffer cloth

typedef struct {
  Vector2 position;
  Vector2 prev_position;
  Vector2 acceleration;
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

Particle create_particle(float x, float y, Color color, bool pinned) {
  Particle p;
  p.position = (Vector2){x, y};
  p.prev_position = (Vector2){x, y};
  p.acceleration = (Vector2){1, GRAVITY};
  p.color = color;
  p.is_pinned = pinned;
  return p;
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
      continue; // Don't move pinned particles

    Vector2 *x = &p->position;
    Vector2 temp = *x;
    Vector2 *x_prev = &p->prev_position;
    Vector2 *a = &p->acceleration;

    // Apply simple friction (0.99f) to stop it from moving forever
    float velocity_x = (x->x - x_prev->x) * 0.99f;
    float velocity_y = (x->y - x_prev->y) * 0.99f;

    x->x += velocity_x + a->x * TIME_STEP * TIME_STEP;
    x->y += velocity_y + a->y * TIME_STEP * TIME_STEP;

    *x_prev = temp;
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

      Vector2 delta = {p2->position.x - p1->position.x,
                       p2->position.y - p1->position.y};

      float delta_length_squared = delta.x * delta.x + delta.y * delta.y;

      if (delta_length_squared < 0.0001f)
        delta_length_squared = 0.0001f;

      float rest_sq = c->rest_length * c->rest_length;
      float factor = rest_sq / (delta_length_squared + rest_sq) - 0.5f;

      delta.x *= factor;
      delta.y *= factor;

      if (!p1->is_pinned && !p2->is_pinned) {
        p1->position.x -= delta.x;
        p1->position.y -= delta.y;
        p2->position.x += delta.x;
        p2->position.y += delta.y;
      } else if (p1->is_pinned && !p2->is_pinned) {
        p2->position.x += delta.x * 2;
        p2->position.y += delta.y * 2;
      } else if (!p1->is_pinned && p2->is_pinned) {
        p1->position.x -= delta.x * 2;
        p1->position.y -= delta.y * 2;
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

  int num_particles = CLOTH_COLS * CLOTH_ROWS;
  // Horizontal constraints + Vertical constraints
  int num_constraints =
      (CLOTH_COLS - 1) * CLOTH_ROWS + (CLOTH_ROWS - 1) * CLOTH_COLS;

  psystem.particles = malloc(sizeof(Particle) * num_particles);
  psystem.constraints = malloc(sizeof(Constraint) * num_constraints);

  // Initialize Particles (Grid)
  for (int y = 0; y < CLOTH_ROWS; y++) {
    for (int x = 0; x < CLOTH_COLS; x++) {
      int index = y * CLOTH_COLS + x;
      float px = START_X + x * SPACING;
      float py = START_Y + y * SPACING;

      // Pin the top row every 5th particle to act like a curtain
      bool pin = (y == 0 && (x % 5 == 0 || x == CLOTH_COLS - 1));

      psystem.particles[index] = create_particle(px, py, PARTICLE_COLOR, pin);
      psystem.particle_count++;
    }
  }

  // Initialize Constraints
  for (int y = 0; y < CLOTH_ROWS; y++) {
    for (int x = 0; x < CLOTH_COLS; x++) {
      int current_idx = y * CLOTH_COLS + x;

      // Connect to Right Neighbor
      if (x < CLOTH_COLS - 1) {
        int right_idx = y * CLOTH_COLS + (x + 1);
        psystem.constraints[psystem.constraint_count] =
            create_constraint(current_idx, right_idx, SPACING);
        psystem.constraint_count++;
      }

      // Connect to Bottom Neighbor
      if (y < CLOTH_ROWS - 1) {
        int down_idx = (y + 1) * CLOTH_COLS + x;
        psystem.constraints[psystem.constraint_count] =
            create_constraint(current_idx, down_idx, SPACING);
        psystem.constraint_count++;
      }
    }
  }

  // Variable to store which particle is being dragged
  int dragged_particle_idx = -1;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(GetColor(0x052A4Fff));

    // --- MOUSE INTERACTION ---
    Vector2 mouse_pos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      float min_dist = 30.0f; // Click radius
      for (int i = 0; i < psystem.particle_count; i++) {
        float dx = mouse_pos.x - psystem.particles[i].position.x;
        float dy = mouse_pos.y - psystem.particles[i].position.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < min_dist) {
          min_dist = dist;
          dragged_particle_idx = i;
        }
      }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      dragged_particle_idx = -1;
    }

    // Update Dragged Particle Position
    if (dragged_particle_idx != -1) {
      Particle *p = &psystem.particles[dragged_particle_idx];
      p->position = mouse_pos;
      p->prev_position = mouse_pos;
    }

    time_step(&psystem);

    // Draw Constraints (Lines)
    for (int i = 0; i < psystem.constraint_count; i++) {
      Constraint c = psystem.constraints[i];
      Vector2 p1 = psystem.particles[c.p1].position;
      Vector2 p2 = psystem.particles[c.p2].position;
      DrawLineV(p1, p2, Fade(CONSTRAINT_COLOR, 0.5f));
    }

    // Draw Particles
    for (int i = 0; i < psystem.particle_count; i++) {
      Particle *p = &psystem.particles[i];
      if (p->is_pinned)
        DrawCircleV(p->position, PARTICLE_RADIUS + 2, RED);
      else
        DrawCircleV(p->position, PARTICLE_RADIUS, p->color);
    }

    DrawText("Hold SPACE for Wind | Drag with MOUSE", 10, 10, 20, RAYWHITE);
    DrawFPS(10, 40);
    EndDrawing();
  }

  free(psystem.particles);
  free(psystem.constraints);
  CloseWindow();
  return 0;
}
