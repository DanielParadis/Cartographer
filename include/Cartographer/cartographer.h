#ifndef CARTOGRAPHER_H
#define CARTOGRAPHER_H

typedef struct CartFrame {
    float u0, u1, v0, v1;
    float ft;
}CartFrame ;

typedef struct CartAnimationMetadata {
    int width;
    int height;
    int count;
    CartFrame *frames;
}CartAnimationMetadata;

typedef struct CartAnimation {
    int id;
    int frame;
    float frameTime;
    float elapsedTime;
}CartAnimation;

extern CartFrame *cartGetFrame(CartAnimation *a);
extern void cartSetAnimation(CartAnimation *a, int id);
extern void cartAnimate(CartAnimation *a, float dt);

#endif