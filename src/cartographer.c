#include "Cartographer/cartographer.h"
extern struct CartAnimationMetadata *cartAnimationMetadata;

CartFrame *cartGetFrame(CartAnimation *a) {
  return &cartAnimationMetadata[a->id].frames[a->frame];
}

void cartSetAnimation(CartAnimation *a, int id) {
  a->elapsedTime = 0;
  a->frameTime = 0;
  a->frame = 0;
  a->id = id;
}

void cartAnimate(CartAnimation *a, float dt) {
  a->elapsedTime += dt;
  a->frameTime += dt;
  if (cartAnimationMetadata[a->id].count > 1 && a->frameTime > cartAnimationMetadata[a->id].frames[a->frame].ft) {
    a->frameTime -= cartAnimationMetadata[a->id].frames[a->frame].ft;
    a->frame = (a->frame + 1) % cartAnimationMetadata[a->id].count;
  }
}