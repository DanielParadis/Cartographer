#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdlib.h>

#define SZ 4196
#define MAXA 16384

char assetDir[4096];
char srcDir[4096];
char incDir[4096];
char atlasDir[4096];

typedef struct Sub {
  IMG_Animation *a;
  SDL_Surface *s;
  char path[4096];
  int x, y;
  int isPNG;
} Sub;
int aCount = 0;

Sub sub[MAXA] = {0};

SDL_Surface *atlas;
int px = 0;
int py = 0;
int ph = 0;

void pushAtlas(char *path, int isPNG) {
  int idx = aCount++;
  if (!isPNG) {
    sub[idx].a = IMG_LoadAnimation(path);
  } else {
    sub[idx].s = IMG_Load(path);
    sub[idx].isPNG = true;
  }
  int l = SDL_strlen(assetDir);
  SDL_snprintf(sub[idx].path, 4096, "ASSET_%s", path + l);
  for (char *c = sub[idx].path; *c; ++c) {
    if (*c == '/' || *c == '\\' || *c == '-')
      *c = '_';
    if (*c == '.') {
      *c = 0;
      break;
    }
  }
  SDL_strupr(sub[idx].path);
  SDL_Log("%s", sub[idx].path);
}

// Callback function for enumeration
SDL_EnumerationResult SDLCALL findGifsCallback(void *userdata,
                                               const char *dirname,
                                               const char *fname) {
  char fullPath[4096];
  SDL_snprintf(fullPath, sizeof(fullPath), "%s%s", dirname, fname);

  SDL_PathInfo info;
  if (SDL_GetPathInfo(fullPath, &info)) {
    // If it's a directory, recurse into it
    if (info.type == SDL_PATHTYPE_DIRECTORY) {
      SDL_EnumerateDirectory(fullPath, findGifsCallback, userdata);
    }
    // If it's a file, check for .gif extension
    else if (info.type == SDL_PATHTYPE_FILE) {
      if (SDL_strcasestr(fname, ".gif")) {
        SDL_Log("Found GIF: %s", fullPath + SDL_strlen(assetDir));
        pushAtlas(fullPath, 0);
      } else if (SDL_strcasestr(fname, ".png") &&
                 !SDL_strcasestr(fname, "atlas.png")) {
        SDL_Log("Found PNG: %s", fullPath + SDL_strlen(assetDir));
        pushAtlas(fullPath, 1);
      }
    }
  }

  return SDL_ENUM_CONTINUE; // Keep searching
}

void searchForGifs() {
  const char *basePath = assetDir; // The application's directory
  if (basePath) {
    SDL_EnumerateDirectory(basePath, findGifsCallback, NULL);
  }
}

void appendAtlas(int index) {
  if (!sub[index].isPNG) {
    IMG_Animation *anim = sub[index].a;
    int frameW = anim->w + 2;
    int frameH = anim->h + 2;
    int frameCount = anim->count;
    int totalWidth = frameW * frameCount;
    if (px + totalWidth >= SZ) { // new row
      px = 0;
      py += ph;
      ph = frameH;
    }
    sub[index].x = px;
    sub[index].y = py;
    for (int i = 0; i < frameCount; i++) {
      SDL_Rect destRect = {px + i * frameW + 1, py + 1, anim->w, anim->h};

      // anim->frames is an array of SDL_Surface pointers
      SDL_BlitSurface(anim->frames[i], NULL, atlas, &destRect);
    }
    px += totalWidth;
  } else {
    SDL_Surface *s = sub[index].s;
    int frameW = s->w + 2;
    int frameH = s->h + 2;
    int frameCount = 1;
    int totalWidth = frameW * frameCount;
    if (px + totalWidth >= SZ) { // new row
      px = 0;
      py += ph;
      ph = frameH;
    }
    sub[index].x = px;
    sub[index].y = py;
    SDL_Rect destRect = {px + 1, py + 1, s->w, s->h};

    // anim->frames is an array of SDL_Surface pointers
    SDL_BlitSurface(s, NULL, atlas, &destRect);
    px += totalWidth;
  }
}

int compareAtlas(const void *a, const void *b) {
  const Sub *a0 = (Sub *)a;
  const Sub *a1 = (Sub *)b;
  int h0, h1;
  if (a0->isPNG)
    h0 = a0->s->h;
  else
    h0 = a0->a->h;
  if (a1->isPNG)
    h1 = a1->s->h;
  else
    h1 = a1->a->h;
  int dif = h1 - h0;
  if (dif)
    return dif;
  if (a0->isPNG)
    h0 = a0->s->w;
  else
    h0 = a0->a->w;
  if (a1->isPNG)
    h1 = a1->s->w;
  else
    h1 = a1->a->w;
  int c0, c1;
  if (a0->isPNG)
    c0 = 1;
  else
    c0 = a0->a->count;
  if (a1->isPNG)
    c1 = 1;
  else
    c1 = a1->a->count;
  return h1 * c1 - h0 * c0;
}

void buildAssetFile() {
  char path[4096];
  SDL_snprintf(path, 4096, "%s%s", incDir, "metadata.h");
  SDL_IOStream *io = SDL_IOFromFile(path, "w");
  char *n = "#ifndef CART_METADATA_H\n"
            "#define CART_METADATA_H\n"
            "#include \"Cartographer/cartographer.h\"\n"
            "typedef enum GFXAssets {\n"
            "ASSET_NULL,\n";
  char *e = "ASSET_MAX\n"
            "} GFXAssets;\n"
            "extern CartAnimationMetadata *cartAnimationMetadata;\n"
            "#endif";
  SDL_WriteIO(io, n, SDL_strlen(n));
  for (int i = 0; i < aCount; i++) {
    char d[4096];
    SDL_snprintf(d, 4096, "%s%s", sub[i].path, ",\n");
    SDL_WriteIO(io, d, SDL_strlen(d));
  }
  SDL_WriteIO(io, e, SDL_strlen(e));
  SDL_CloseIO(io);
  SDL_snprintf(path, 4096, "%s%s", srcDir, "metadata.c");
  io = SDL_IOFromFile(path, "w");
  char *head = "#include \"Cartographer/metadata.h\"\n"
               "CartAnimationMetadata *cartAnimationMetadata = (CartAnimationMetadata[]){\n"
               "{0},\n";
  SDL_WriteIO(io, head, SDL_strlen(head));
  for (int i = 0; i < aCount; i++) {
    if (!sub[i].isPNG) {
      SDL_IOprintf(io,
                   "{\n"
                   "%d, %d, %d,\n"
                   "(CartFrame[]){\n",
                   sub[i].a->w, sub[i].a->h, sub[i].a->count);
      for (int j = 0; j < sub[i].a->count; j++) {
        int x0 = sub[i].x + j * (sub[i].a->w + 2) + 1;
        int x1 = x0 + sub[i].a->w;
        int y0 = sub[i].y + 1;
        int y1 = y0 + sub[i].a->h;
        SDL_IOprintf(
            io, "{%d.0f/%d.0f, %d.0f/%d.0f, %d.0f/%d.0f, %d.0f/%d.0f, %f},\n",
            x0, SZ, x1, SZ, y0, SZ, y1, SZ,
            (float)sub[i].a->delays[j] / 1000.0f);
      }
    } else {
      SDL_IOprintf(io,
                   "{\n"
                   "%d, %d, %d,\n"
                   "(CartFrame[]){\n",
                   sub[i].s->w, sub[i].s->h, 1);
      int x0 = sub[i].x + 1;
      int x1 = x0 + sub[i].s->w;
      int y0 = sub[i].y + 1;
      int y1 = y0 + sub[i].s->h;
      SDL_IOprintf(
          io, "{%d.0f/%d.0f, %d.0f/%d.0f, %d.0f/%d.0f, %d.0f/%d.0f, %f},\n", x0,
          SZ, x1, SZ, y0, SZ, y1, SZ, 0.0);
    }
    SDL_IOprintf(io, "}\n},\n");
  }
  SDL_IOprintf(io, "};\n");
}

int handleArgs(int argc, char **argv) {
  if (argc != 9) {
    SDL_Log("Invalid number of arguments. Cartographer expects the following:\n"
            "cartographer --assets \"asset/path\" --srcdir "
            "\"path/to/generated/data\" --atlas \"atlas/path/and/name.png\"");
    return 0;
  }
  int i = 1;
  while (i != 9) {
    char *s = argv[i];
    char *dest = NULL;
    int isDirectory = 1;
    if (SDL_strcasecmp(s, "--assets") == 0) {
      dest = assetDir;
    } else if (SDL_strcasecmp(s, "--srcdir") == 0) {
      dest = srcDir;
    } else if (SDL_strcasecmp(s, "--incdir") == 0) {
      dest = incDir;
    } else if (SDL_strcasecmp(s, "--atlas") == 0) {
      dest = atlasDir;
      isDirectory = 0;
    } else {
      SDL_Log("Cartographer invalid argument: %s", s);
      SDL_Log("Expected a flag: --assets, --srcdir, --incdir, --atlas");
      return 0;
    }
    if (!SDL_GetPathInfo(argv[i + 1], NULL) && isDirectory) {
      SDL_Log("Cartographer path error. Path doesn't exist: %s", argv[i + 1]);
      return 0;
    }
    SDL_strlcpy(dest, argv[i + 1], 4096);
    if (isDirectory) {
      int srcLen = SDL_strlen(dest);
      if (dest[srcLen - 1] != '/') {
        dest[srcLen] = '/';
        dest[srcLen + 1] = '\0';
      }
    }
    i += 2;
  }
  return 1;
}

int main(int argc, char **argv) {
  if (!handleArgs(argc, argv))
    return 1;
  SDL_Init(0);
  atlas = SDL_CreateSurface(SZ, SZ, SDL_PIXELFORMAT_RGBA32);
  searchForGifs();
  qsort(sub, aCount, sizeof(sub[0]), compareAtlas);
  if (!sub[0].isPNG)
    ph = sub[0].a->h + 2;
  else
    ph = sub[0].s->h + 2;
  for (int i = 0; i < aCount; i++) {
    appendAtlas(i);
  }
  char atlasPath[4096];
  IMG_SavePNG(atlas, atlasDir);
  buildAssetFile();
  SDL_DestroySurface(atlas);
  for (int i = 0; i < aCount; i++)
    if (!sub[i].isPNG)
      IMG_FreeAnimation(sub[i].a);
    else
      SDL_DestroySurface(sub[i].s);
  return 0;
}