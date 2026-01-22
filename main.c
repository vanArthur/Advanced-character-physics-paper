#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

#define WIDTH 1000
#define HEIGHT 1000

#define PARTICLE_RADIUS 5
#define PARTICLE_COLOR GetColor(0xFF6F61ff)
#define GRAVITY 1.f
#define TIME_STEP 0.16f
#define NUM_ITERATIONS 5

typedef struct {
  Vector2 position;
  Vector2 prev_position;
  Vector2 acceleration;
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

Particle create_particle(float x, float y, Color color) {
  Particle p;
  p.position = (Vector2){x, y};
  p.prev_position = (Vector2){x, y};
  p.acceleration = (Vector2){1, GRAVITY};
  p.color = color;
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

    Vector2 *x = &p->position;
    Vector2 temp = *x;
    Vector2 *x_prev = &p->prev_position;
    Vector2 *a = &p->acceleration;
    x->x += (x->x - x_prev->x) + a->x * TIME_STEP * TIME_STEP;
    x->y += (x->y - x_prev->y) + a->y * TIME_STEP * TIME_STEP;
    *x_prev = temp;
  }
}

void accumulate_forces(ParticleSystem *psystem) {
  for (int i = 0; i < psystem->particle_count; i++) {
    Particle *p = &psystem->particles[i];
    p->acceleration.y = GRAVITY; // reset acceleration to gravity
  }
}

void satisfy_constraints(ParticleSystem *psystem, int width, int height) {
  for (int i = 0; i < NUM_ITERATIONS; i++) {

    for (int j = 0; j < psystem->particle_count; j++) {
      Particle *p = &psystem->particles[j];
      Vector2 *x = &p->position;
      *x = Vector2Clamp(
          *x, (Vector2){PARTICLE_RADIUS, PARTICLE_RADIUS},
          (Vector2){width - PARTICLE_RADIUS, height - PARTICLE_RADIUS});
    }

    for (int i = 0; i < psystem->constraint_count; i++) {
      Constraint *c = &psystem->constraints[i];
      Particle *p1 = &psystem->particles[c->p1];
      Particle *p2 = &psystem->particles[c->p2];

      Vector2 delta = {p2->position.x - p1->position.x,
                       p2->position.y - p1->position.y};
      float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);
      float diff = (dist - c->rest_length) / dist;

      p1->position.x += delta.x * 0.5f * diff;
      p1->position.y += delta.y * 0.5f * diff;
      p2->position.x -= delta.x * 0.5f * diff;
      p2->position.y -= delta.y * 0.5f * diff;
    }
  }
}

void time_step(ParticleSystem *psystem, int width, int height) {
  accumulate_forces(psystem);
  verlet(psystem);
  satisfy_constraints(psystem, width, height);
}

int main(void) {
  InitWindow(WIDTH, HEIGHT, "Advanced Character Physics");
  SetTargetFPS(60);

  int w = GetScreenWidth();
  int h = GetScreenHeight();

  psystem.particles = malloc(sizeof(Particle) * 2);
  for (int i = 0; i < 2; i++) {
    // create particles at random positions
    float x = GetRandomValue(0, w);
    float y = GetRandomValue(0, h / 2);
    psystem.particles[psystem.particle_count] = create_particle(x, y, PARTICLE_COLOR);
    psystem.particle_count++;
  }

  // create a constraint between the two particles
  psystem.constraints = malloc(sizeof(Constraint) * 1);
  psystem.constraints[psystem.constraint_count] = create_constraint(0, 1, 200.f);
  psystem.constraint_count++;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(GetColor(0x052A4Fff));

    time_step(&psystem, w, h);

    // draw particles
    for (int i = 0; i < psystem.particle_count; i++) {
      Particle *p = &psystem.particles[i];
      DrawCircleV(p->position, PARTICLE_RADIUS, p->color);
    }
    EndDrawing();
  }

  free(psystem.particles);

  CloseWindow();
  return 0;
}
