// texc.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "dds.h"
#include "OptionParser.h"
#include "job.h"
using namespace optparse;

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

enum PlatformType {
  PLATFORM_PC,
  PLATFORM_PS4,
};

struct Arguments {
  char input_filename[256];
  char output_filename[256];
  FILE* output_file;
  PlatformType platform;
  int flags;
  bool no_compress;
  bool premultiply_alpha;
  bool mipmap;
  bool fast;
  bool verbose;
} g_args;

static Gnm::Texture g_gnm_texture;
static JobQueue g_job_queue(4096);
static int g_request_exit = 0;
static std::atomic_int g_num_jobs;

void usage() {
  printf("Usage:\n"
         "\ttexc [input_filename]\n");
}

static int compute_mipmap_count(int w, int h) {
  int n = 1;
  while (w > 1 && h > 1) {
    w = max(1, w / 2);
    h = max(1, h / 2);
    ++n;
  }
  return n;
}
static int compute_dds_linear_size(int w, int h) {
  if (g_args.no_compress) return w * h * 4;
  else if (g_args.flags & squish::kDxt1) return max(1, ((w + 3) / 4)) * 8 * ((h + 3) / 4);
  else return max(1, ((w + 3) / 4)) * 16 * ((h + 3) / 4);
}

void write_dds_header(int w, int h) {
  FILE* f = g_args.output_file;
  fseek(f, 0, SEEK_SET);
  fwrite(&DDS_MAGIC, 4, 1, f);
  DDS_HEADER header;
  memset(&header, 0, sizeof(header));
  header.dwSize = sizeof(header);
  header.dwFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE;
  if (g_args.mipmap) header.dwFlags |= DDS_HEADER_FLAGS_MIPMAP;
  header.dwWidth = w;
  header.dwHeight = h;
  header.dwPitchOrLinearSize = compute_dds_linear_size(w, h);
  header.dwDepth = 1;
  if (g_args.mipmap) header.dwMipMapCount = compute_mipmap_count(w, h);
  if (g_args.no_compress) header.ddspf = DDSPF_A8R8G8B8;
  else if (g_args.flags & squish::kDxt1) header.ddspf = DDSPF_DXT1;
  else if (g_args.flags & squish::kDxt3) header.ddspf = DDSPF_DXT3;
  else if (g_args.flags & squish::kDxt5) header.ddspf = DDSPF_DXT5;
  header.dwCaps = DDS_SURFACE_FLAGS_TEXTURE;
  if (g_args.mipmap) header.dwCaps |= DDS_SURFACE_FLAGS_MIPMAP;
  fwrite(&header, sizeof(header), 1, f);
  fflush(f);
}

void write_dds_data(int w, int h, int mip, const void* image_data, int image_size) {
  FILE* f = g_args.output_file;
  size_t offset = 4 + sizeof(DDS_HEADER);
  int mip_w = w;
  int mip_h = h;
  for (int i = 0; i < mip; ++i) {
    offset += compute_dds_linear_size(mip_w, mip_h);
    mip_w = max(1, mip_w / 2);
    mip_h = max(1, mip_h / 2);
  }
  fseek(f, (long)offset, SEEK_SET);
  fwrite(image_data, image_size, 1, f);
  fflush(f);
}

int compute_gnf_stream_size(int w, int h) {
  int num_mips = g_args.mipmap ? compute_mipmap_count(w, h) : 1;
  Gnm::DataFormat data_format = Gnm::kDataFormatB8G8R8A8Unorm;
  Gnm::TileMode tile_mode = Gnm::kTileModeThin_1dThin;
  if (g_args.flags & squish::kDxt1) data_format = Gnm::kDataFormatBc1Unorm;
  else if (g_args.flags & squish::kDxt3) data_format = Gnm::kDataFormatBc2Unorm;
  else if (g_args.flags & squish::kDxt5) data_format = Gnm::kDataFormatBc3Unorm;
  Gnm::Texture texture;
  texture.initAs2d(w, h, num_mips, data_format, tile_mode, Gnm::kNumFragments1);
  uint64_t tiled_size;
  Gnm::AlignmentType tiled_align;
  int ret = GpuAddress::computeTotalTiledTextureSize(&tiled_size, &tiled_align, &texture);
  if (ret != 0) {
    printf("Failed to computeTotalTiledTextureSize.\n");
    exit(-1);
  }
  return 256 + (int)tiled_size;
}

void write_gnf_header(int w, int h) {
  size_t gnf_file_size = 256;
  Gnf::GnfFile* gnf_file = (Gnf::GnfFile*)malloc(gnf_file_size);
  memset(gnf_file, 0, gnf_file_size);
  gnf_file->header.m_magicNumber = Gnf::kMagic;
  gnf_file->header.m_contentsSize = 248;
  gnf_file->contents.m_version = Gnf::kVersion;
  gnf_file->contents.m_numTextures = 1;
  gnf_file->contents.m_alignment = 8;
  gnf_file->contents.m_streamSize = compute_gnf_stream_size(w, h);
  
  int num_mips = g_args.mipmap ? compute_mipmap_count(w, h) : 1;
  Gnm::DataFormat data_format = Gnm::kDataFormatB8G8R8A8Unorm;
  Gnm::TileMode tile_mode = Gnm::kTileModeThin_1dThin;
  if (g_args.flags & squish::kDxt1) data_format = Gnm::kDataFormatBc1Unorm;
  else if (g_args.flags & squish::kDxt3) data_format = Gnm::kDataFormatBc2Unorm;
  else if (g_args.flags & squish::kDxt5) data_format = Gnm::kDataFormatBc3Unorm;
  auto& texture = gnf_file->contents.m_textures[0];
  texture.initAs2d(w, h, num_mips, data_format,
                   tile_mode, Gnm::kNumFragments1);
  g_gnm_texture = texture;
  
  texture.m_regs[7] = gnf_file->contents.m_streamSize - 256;
  texture.m_regs[1] = (texture.m_regs[1] & 0xffffff00) | 8;

  FILE* f = g_args.output_file;
  fseek(f, 0, SEEK_SET);
  fwrite(gnf_file, gnf_file_size, 1, f);
  fflush(f);
  free(gnf_file);
}

void write_gnf_data(int w, int h, int mip, const void* image_data, int image_size) {
  FILE* f = g_args.output_file;
  Gnm::TileMode tile_mode = Gnm::kTileModeThin_1dThin;
  uint64_t surface_offset, surface_size;
  uint64_t untile_size;
  Gnm::AlignmentType untile_align;
  GpuAddress::TilingParameters tp;
  tp.initFromTexture(&g_gnm_texture, mip, 0);
  GpuAddress::computeTextureSurfaceOffsetAndSize(&surface_offset, &surface_size, &g_gnm_texture, mip, 0);
  GpuAddress::computeUntiledSurfaceSize(&untile_size, &untile_align, &tp);

  if (untile_size != image_size) {
    printf("Untiled size mismatch, expect %d, got %d.\n", (int)untile_size, image_size);
    exit(-1);
  }

  fseek(f, (long)surface_offset + 256, SEEK_SET);
  void* tiled_data = malloc(surface_size);
  GpuAddress::tileSurface(tiled_data, image_data, &tp);
  fwrite(tiled_data, surface_size, 1, f);
  free(tiled_data);
  fflush(f);
}

void write_image_header(int w, int h) {
  if (g_args.platform == PLATFORM_PC) write_dds_header(w, h);
  else if (g_args.platform == PLATFORM_PS4) write_gnf_header(w, h);
}

void write_image_data(int w, int h, int mip, const void* image_data, int image_size) {
  if (g_args.platform == PLATFORM_PC) write_dds_data(w, h, mip, image_data, image_size);
  else if (g_args.platform == PLATFORM_PS4) write_gnf_data(w, h, mip, image_data, image_size);
}

void image_bgra_to_rgba(void* in_data, int w, int h) {
  uint8_t* data = (uint8_t*)in_data;
  const int pitch = w * 4;
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; ++j) {
      std::swap(data[j * 4], data[j * 4 + 2]);
    }
    data += pitch;
  }
}

void compress_thread(JobQueue& job_queue) {
  Job job;
  squish::u8 rgba[64];
  while (!g_request_exit) {
    if (job_queue.dequeue(job)) {
      squish::u8* p = rgba;
      for (int py = 0; py < 4; ++py) {
        for (int px = 0; px < 4; ++px) {
          int offset = 4 * py + px;
          if (job.mask & (1 << offset)) {
            memcpy(rgba + offset * 4, job.ibuf + py * job.pitch + px * 4, 4);
          }
        }
      }
      squish::CompressMasked(rgba, job.mask, job.obuf, job.flags);
      --g_num_jobs;
    } else std::this_thread::yield();
  }
}

void create_compress_threads() {
  g_num_jobs.store(0);
  int n = std::thread::hardware_concurrency();
  for (int i = 0; i < n; ++i) {
    std::thread t(compress_thread, std::ref(g_job_queue));
    t.detach();
  }
}

void compress_image(const void* in_data, int width, int height, void* out_data) {
  if (width < 256 && height < 256) {
    squish::CompressImage((const squish::u8*)in_data, width, height, out_data, g_args.flags);
    return;
  }
  // initialise the block output
  const uint8_t* src = reinterpret_cast<const uint8_t*>(in_data);
  uint8_t* dst = reinterpret_cast<uint8_t*>(out_data);
  int bytes_per_block = ((g_args.flags & squish::kDxt1) != 0) ? 8 : 16;

  auto src_pitch = width * 4;
  for (int y = 0, m = height / 4; y < m; ++y) {
    for (int x = 0, n = width / 4; x < n; ++x) {
      Job job(src + (y * 4 * src_pitch + x * 4 * 4), src_pitch,
              dst + (y * n + x) * bytes_per_block, g_args.flags);
      ++g_num_jobs;
      while (!g_job_queue.enqueue(job))
        ;
    }
  }

  while (g_num_jobs.load() > 0)
    ;
}

void compress() {
  std::chrono::time_point<std::chrono::system_clock> start_time, end_time;

  start_time = std::chrono::system_clock::now();

  auto fif = FreeImage_GetFIFFromFilename(g_args.input_filename);
  if (fif == FIF_UNKNOWN) {
    printf("Unknown file format %s.\n", g_args.input_filename);
    exit(-1);
  }
  FIBITMAP* orig_bitmap = FreeImage_Load(fif, g_args.input_filename);
  FIBITMAP* bitmap = orig_bitmap;
  if (!orig_bitmap) {
    printf("Unable to load %s.\n", g_args.input_filename);
    exit(-1);
  }
  auto w = FreeImage_GetWidth(orig_bitmap);
  auto h = FreeImage_GetHeight(orig_bitmap);
  if (w & 3 || h & 3) {
    printf("Warning: Image size %dx%d not divisible by 4, not bcX compressible\n", w, h);
    g_args.no_compress = true;
    g_args.flags = 0;
  }
  BYTE* in_data = (BYTE*)malloc(w * h * 4);
  int out_size = g_args.no_compress ? w * h * 4 : squish::GetStorageRequirements(w, h, g_args.flags);
  unsigned char* out_data = (unsigned char*)malloc(out_size);
  if (g_args.premultiply_alpha) FreeImage_PreMultiplyWithAlpha(bitmap);
  FreeImage_ConvertToRawBits(in_data, bitmap, w * 4, 32,
                             FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
  write_image_header(w, h);
  if (g_args.no_compress) {
    memcpy(out_data, in_data, out_size);
    write_image_data(w, h, 0, out_data, out_size);
    if (g_args.mipmap) {
      int mipmap_count = compute_mipmap_count(w, h);
      int mipmap_width = w;
      int mipmap_height = h;
      for (int mip = 1; mip < mipmap_count; ++mip) {
        mipmap_width = max(1, mipmap_width / 2);
        mipmap_height = max(1, mipmap_height / 2);
        bitmap = FreeImage_Rescale(orig_bitmap, mipmap_width, mipmap_height, FILTER_BICUBIC);
        FreeImage_ConvertToRawBits(in_data, bitmap, mipmap_width * 4, 32,
                                   FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
        out_size = mipmap_width * mipmap_height * 4;
        memcpy(out_data, in_data, out_size);
        write_image_data(w, h, mip, out_data, out_size);
      }
    }
  } else {
    image_bgra_to_rgba(in_data, w, h);
    compress_image(in_data, w, h, out_data);
    write_image_data(w, h, 0, out_data, out_size);
    if (g_args.mipmap) {
      int mipmap_count = compute_mipmap_count(w, h);
      int mipmap_width = w;
      int mipmap_height = h;
      for (int mip = 1; mip < mipmap_count; ++mip) {
        mipmap_width = max(1, mipmap_width / 2);
        mipmap_height = max(1, mipmap_height / 2);
        bitmap = FreeImage_Rescale(bitmap, mipmap_width, mipmap_height, FILTER_BICUBIC);
        FreeImage_ConvertToRawBits(in_data, bitmap, mipmap_width * 4, 32,
                                   FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
        image_bgra_to_rgba(in_data, mipmap_width, mipmap_height);
        compress_image(in_data, mipmap_width, mipmap_height, out_data);
        out_size = squish::GetStorageRequirements(mipmap_width, mipmap_height, g_args.flags);
        write_image_data(w, h, mip, out_data, out_size);
      }
    }
  }

  end_time = std::chrono::system_clock::now();
  std::chrono::duration<float> duration = end_time - start_time;
  if (g_args.verbose) printf("Time cost: %.3f s\n", duration.count());
}

void parse_arguments(int argc, char* argv[]) {
  OptionParser parser;
  parser.prog("texc");
  parser.usage("%prog [options] input_filename");
  parser.description("Compress <input_filename> to platform dependent texture file.");
  parser.add_option("-v", "--verbose").action("store_true").dest("verbose").help("output verbose info");
  parser.add_option("-f", "--fast").action("store_true").dest("fast").help("use fast but low quality color compressor");
  parser.add_option("-m", "--mipmap").action("store_true").dest("mipmap").help("generate mipmap");
  parser.add_option("--pc").action("store_const").dest("platform").set_const("pc").help("output DDS texture");
  parser.add_option("--ps4").action("store_const").dest("platform").set_const("ps4").help("output GNF texture");
  parser.add_option("--bc1").action("store_true").dest("bc1").help("DXT1 compression");
  parser.add_option("--bc2").action("store_true").dest("bc2").help("DXT3 compression");
  parser.add_option("--bc3").action("store_true").dest("bc3").help("DXT5 compression");
  parser.add_option("--no-pm").action("store_true").dest("no_pm_alpha").help("Disable premultiply-alpha");
  auto options = parser.parse_args(argc, (const char**)argv);
  auto args = parser.args();

  g_args.platform = strcmp(options.get("platform"), "ps4") == 0 ? PLATFORM_PS4 : PLATFORM_PC;
  if (args.size() != 1) {
    printf("Parse argument failed: only accept one input filename.\n");
    exit(-1);
  }
  strcpy(g_args.input_filename, args.front().c_str());
  g_args.output_filename[0] = '\0';
  g_args.verbose = options.is_set("verbose");
  g_args.mipmap = options.is_set("mipmap");
  g_args.fast = options.is_set("fast");
  g_args.flags = g_args.fast ? 0 : squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha;
  g_args.no_compress = false;
  g_args.premultiply_alpha = true;
  if (options.is_set("bc1")) g_args.flags |= squish::kDxt1;
  else if (options.is_set("bc2")) g_args.flags |= squish::kDxt3;
  else if (options.is_set("bc3")) g_args.flags |= squish::kDxt5;
  else g_args.no_compress = true;

  if (options.is_set("no_pm_alpha")) g_args.premultiply_alpha = false;

  strcpy(g_args.output_filename, g_args.input_filename);
  char* p = strrchr(g_args.output_filename, '.');
  if (!p) {
    printf("Invalid filename: %s\n", g_args.input_filename);
    exit(-1);
  }
  if (g_args.platform == PLATFORM_PC) strcpy(p, ".dds");
  else if (g_args.platform == PLATFORM_PS4) strcpy(p, ".gnf");
  g_args.output_file = fopen(g_args.output_filename, "wb");
  if (!g_args.output_file) {
    printf("Failed to open output file %s\n", g_args.output_filename);
    exit(-1);
  }
}

int main(int argc, char* argv[]) {
  FreeImage_Initialise();
  parse_arguments(argc, argv);
  create_compress_threads();
  compress();
  FreeImage_DeInitialise();
  return 0;
}
