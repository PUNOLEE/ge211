#include "ge211_resource.h"
#include "ge211_error.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>

namespace ge211 {

using namespace detail;

static const char* search_prefixes[] = {
        "Resources/",
        "../Resources/",
        GE211_RESOURCES
};

namespace detail {

static void close_rwops(SDL_RWops* rwops)
{
    SDL_RWclose(rwops);
}

delete_ptr<SDL_RWops> File_resource::open_rwops_(const std::string& filename)
{
    for (auto prefix : search_prefixes) {
        std::string path;
        path += prefix;
        path += filename;
        SDL_RWops* rwops = SDL_RWFromFile(path.c_str(), "rb");
        if (rwops) return {rwops, &close_rwops};
    }

    throw File_error::could_not_open(filename);
}

File_resource::File_resource(const std::string& filename)
        : ptr_{open_rwops_(filename)}
{ }

} // end namespace detail

delete_ptr<TTF_Font> Font::load_(const std::string& filename,
                                 File_resource& file,
                                 int size)
{
    TTF_Font* result = TTF_OpenFontRW(file.get_raw_(), 0, size);
    if (result) return {result, &TTF_CloseFont};

    throw Font_error::could_not_load(filename);
}

Font::Font(const std::string& filename, int size)
        : file_{filename},
          ptr_{load_(filename, file_, size)}
{ }

}