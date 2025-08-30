#include "GenesisPlusGX.h"

#include "SDL.h"

extern "C"
{
    #include "shared.h"
    #include "sms_ntsc.h"
    #include "md_ntsc.h"
}

int sdl_input_update()
{
    int joynum = 0;
    const std::uint8_t *keystate = SDL_GetKeyboardState(nullptr);
    /* reset input */
    input.pad[joynum] = 0;

    switch (input.dev[joynum])
    {
        case DEVICE_LIGHTGUN:
        {
            /* get mouse coordinates (absolute values) */
            int x,y;
            int state = SDL_GetMouseState(&x,&y);

            /* X axis */
            //input.analog[joynum][0] =  x - (sdl_video.surf_screen->w-bitmap.viewport.w)/2;

            /* Y axis */
            //input.analog[joynum][1] =  y - (sdl_video.surf_screen->h-bitmap.viewport.h)/2;

            /* TRIGGER, B, C (Menacer only), START (Menacer & Justifier only) */
            if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_A;
            if(state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_B;
            if(state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_C;
            if(keystate[SDL_SCANCODE_F])  input.pad[joynum] |= INPUT_START;
            break;
        }

        case DEVICE_PADDLE:
        {
            /* get mouse (absolute values) */
            int x;
            int state = SDL_GetMouseState(&x, NULL);

            /* Range is [0;256], 128 being middle position */
            //input.analog[joynum][0] = x * 256 /sdl_video.surf_screen->w;

            /* Button I -> 0 0 0 0 0 0 0 I*/
            if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;

            break;
        }

        case DEVICE_SPORTSPAD:
        {
            /* get mouse (relative values) */
            int x,y;
            int state = SDL_GetRelativeMouseState(&x,&y);

            /* Range is [0;256] */
            input.analog[joynum][0] = (unsigned char)(-x & 0xFF);
            input.analog[joynum][1] = (unsigned char)(-y & 0xFF);

            /* Buttons I & II -> 0 0 0 0 0 0 II I*/
            if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
            if(state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;

            break;
        }

        case DEVICE_MOUSE:
        {
            /* get mouse (relative values) */
            int x,y;
            int state = SDL_GetRelativeMouseState(&x,&y);

            /* Sega Mouse range is [-256;+256] */
            input.analog[joynum][0] = x * 2;
            input.analog[joynum][1] = y * 2;

            /* Vertical movement is upsidedown */
            if (!config.invert_mouse)
            input.analog[joynum][1] = 0 - input.analog[joynum][1];

            /* Start,Left,Right,Middle buttons -> 0 0 0 0 START MIDDLE RIGHT LEFT */
            if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
            if(state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;
            if(state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_A;
            if(keystate[SDL_SCANCODE_F])  input.pad[joynum] |= INPUT_START;

            break;
        }

        case DEVICE_XE_1AP:
        {
            /* A,B,C,D,Select,START,E1,E2 buttons -> E1(?) E2(?) START SELECT(?) A B C D */
            if(keystate[SDL_SCANCODE_A])  input.pad[joynum] |= INPUT_START;
            if(keystate[SDL_SCANCODE_S])  input.pad[joynum] |= INPUT_A;
            if(keystate[SDL_SCANCODE_D])  input.pad[joynum] |= INPUT_C;
            if(keystate[SDL_SCANCODE_F])  input.pad[joynum] |= INPUT_Y;
            if(keystate[SDL_SCANCODE_Z])  input.pad[joynum] |= INPUT_B;
            if(keystate[SDL_SCANCODE_X])  input.pad[joynum] |= INPUT_X;
            if(keystate[SDL_SCANCODE_C])  input.pad[joynum] |= INPUT_MODE;
            if(keystate[SDL_SCANCODE_V])  input.pad[joynum] |= INPUT_Z;

            /* Left Analog Stick (bidirectional) */
            if(keystate[SDL_SCANCODE_UP])     input.analog[joynum][1]-=2;
            else if(keystate[SDL_SCANCODE_DOWN])   input.analog[joynum][1]+=2;
            else input.analog[joynum][1] = 128;
            if(keystate[SDL_SCANCODE_LEFT])   input.analog[joynum][0]-=2;
            else if(keystate[SDL_SCANCODE_RIGHT])  input.analog[joynum][0]+=2;
            else input.analog[joynum][0] = 128;

            /* Right Analog Stick (unidirectional) */
            if(keystate[SDL_SCANCODE_KP_8])    input.analog[joynum+1][0]-=2;
            else if(keystate[SDL_SCANCODE_KP_2])   input.analog[joynum+1][0]+=2;
            else if(keystate[SDL_SCANCODE_KP_4])   input.analog[joynum+1][0]-=2;
            else if(keystate[SDL_SCANCODE_KP_6])  input.analog[joynum+1][0]+=2;
            else input.analog[joynum+1][0] = 128;

            /* Limiters */
            if (input.analog[joynum][0] > 0xFF) input.analog[joynum][0] = 0xFF;
            else if (input.analog[joynum][0] < 0) input.analog[joynum][0] = 0;
            if (input.analog[joynum][1] > 0xFF) input.analog[joynum][1] = 0xFF;
            else if (input.analog[joynum][1] < 0) input.analog[joynum][1] = 0;
            if (input.analog[joynum+1][0] > 0xFF) input.analog[joynum+1][0] = 0xFF;
            else if (input.analog[joynum+1][0] < 0) input.analog[joynum+1][0] = 0;
            if (input.analog[joynum+1][1] > 0xFF) input.analog[joynum+1][1] = 0xFF;
            else if (input.analog[joynum+1][1] < 0) input.analog[joynum+1][1] = 0;

            break;
        }

        case DEVICE_PICO:
        {
            /* get mouse (absolute values) */
            int x,y;
            int state = SDL_GetMouseState(&x,&y);

            /* Calculate X Y axis values */
            //input.analog[0][0] = 0x3c  + (x * (0x17c-0x03c+1)) / sdl_video.surf_screen->w;
            //input.analog[0][1] = 0x1fc + (y * (0x2f7-0x1fc+1)) / sdl_video.surf_screen->h;

            /* Map mouse buttons to player #1 inputs */
            if(state & SDL_BUTTON_MMASK) pico_current = (pico_current + 1) & 7;
            if(state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_PICO_RED;
            if(state & SDL_BUTTON_LMASK) input.pad[0] |= INPUT_PICO_PEN;

            break;
        }

        case DEVICE_TEREBI:
        {
            /* get mouse (absolute values) */
            int x,y;
            int state = SDL_GetMouseState(&x,&y);

            /* Calculate X Y axis values */
            //input.analog[0][0] = (x * 250) / sdl_video.surf_screen->w;
            //input.analog[0][1] = (y * 250) / sdl_video.surf_screen->h;

            /* Map mouse buttons to player #1 inputs */
            if(state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_B;

            break;
        }

        case DEVICE_GRAPHIC_BOARD:
        {
            /* get mouse (absolute values) */
            int x,y;
            int state = SDL_GetMouseState(&x,&y);

            /* Calculate X Y axis values */
            //input.analog[0][0] = (x * 255) / sdl_video.surf_screen->w;
            //input.analog[0][1] = (y * 255) / sdl_video.surf_screen->h;

            /* Map mouse buttons to player #1 inputs */
            if(state & SDL_BUTTON_LMASK) input.pad[0] |= INPUT_GRAPHIC_PEN;
            if(state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_GRAPHIC_MENU;
            if(state & SDL_BUTTON_MMASK) input.pad[0] |= INPUT_GRAPHIC_DO;

            break;
        }

        case DEVICE_ACTIVATOR:
        {
            if(keystate[SDL_SCANCODE_G])  input.pad[joynum] |= INPUT_ACTIVATOR_7L;
            if(keystate[SDL_SCANCODE_H])  input.pad[joynum] |= INPUT_ACTIVATOR_7U;
            if(keystate[SDL_SCANCODE_J])  input.pad[joynum] |= INPUT_ACTIVATOR_8L;
            if(keystate[SDL_SCANCODE_K])  input.pad[joynum] |= INPUT_ACTIVATOR_8U;
        }

        default:
        {
            if(keystate[SDL_SCANCODE_A])  input.pad[joynum] |= INPUT_A;
            if(keystate[SDL_SCANCODE_S])  input.pad[joynum] |= INPUT_B;
            if(keystate[SDL_SCANCODE_D])  input.pad[joynum] |= INPUT_C;
            if(keystate[SDL_SCANCODE_F])  input.pad[joynum] |= INPUT_START;
            if(keystate[SDL_SCANCODE_Z])  input.pad[joynum] |= INPUT_X;
            if(keystate[SDL_SCANCODE_X])  input.pad[joynum] |= INPUT_Y;
            if(keystate[SDL_SCANCODE_C])  input.pad[joynum] |= INPUT_Z;
            if(keystate[SDL_SCANCODE_V])  input.pad[joynum] |= INPUT_MODE;

            if(keystate[SDL_SCANCODE_UP]) input.pad[joynum] |= INPUT_UP;
            else
            if(keystate[SDL_SCANCODE_DOWN]) input.pad[joynum] |= INPUT_DOWN;
            if(keystate[SDL_SCANCODE_LEFT]) input.pad[joynum] |= INPUT_LEFT;
            else
            if(keystate[SDL_SCANCODE_RIGHT]) input.pad[joynum] |= INPUT_RIGHT;

            break;
        }
    }
    return 1;
}

void GenesisPlusGX::Initialize()
{
    // set default config
    error_init();
    set_config_defaults();

    // mark all BIOS as unloaded
    system_bios = 0;

    // Genesis BOOT ROM support (2KB max)
    memset(boot_rom, 0xFF, 0x800);
}

void GenesisPlusGX::Shutdown()
{
    audio_shutdown();
    error_shutdown();
}

void GenesisPlusGX::Reset(bool Hard)
{
    system_reset();
}

std::string GenesisPlusGX::GetMediaFilter(int MediaSource)
{
    return "Genesis Plus GX (*.md *.chd){.md,.chd}";
}

std::error_code GenesisPlusGX::InsertMediaSource(std::string_view Path, int MediaSource)
{
    memset(&bitmap, 0, sizeof(t_bitmap));
    bitmap.width = 720;
    bitmap.height = 576;
    bitmap.pitch = (bitmap.width * 4);

    m_FrameBuffer.resize(bitmap.width * bitmap.height);
    bitmap.data = reinterpret_cast<uint8*>(m_FrameBuffer.data());
    bitmap.viewport.changed = 3;

    if(!load_rom(const_cast<char*>(Path.data())))
    {
        return std::make_error_code(std::errc::invalid_argument);
    }

    audio_init(48000, 0);
    system_init();
    system_reset();

    return {};
}

void GenesisPlusGX::RemoveMediaSource(int MediaSource)
{
}

void GenesisPlusGX::PlugController(int Port, int ControllerType)
{
}

void GenesisPlusGX::UnplugController(int Port)
{
}

void GenesisPlusGX::SetControllerInputValue(int Port, int Input, float Value)
{
}

void GenesisPlusGX::SetControllerInputValues(int Port, std::span<float> Values)
{
}

double GenesisPlusGX::GetRefreshUpdate()
{
    return vdp_pal ? 50.0 : 60.0;
}

void GenesisPlusGX::DoFrame()
{
    if (system_hw == SYSTEM_MCD)
    {
        system_frame_scd(0);
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
    {
        system_frame_gen(0);
    }
    else
    {
        system_frame_sms(0);
    }

    if (RenderFunc != nullptr)
    {
        std::vector<uint32_t> FrameBuffer(bitmap.viewport.w * bitmap.viewport.h);

        for (int X = 0; X < bitmap.viewport.w; X++)
        {
            for (int Y = 0; Y < bitmap.viewport.h; Y++)
            {
                FrameBuffer[X + Y * bitmap.viewport.w] = reinterpret_cast<std::uint32_t*>(bitmap.data)[(Y + bitmap.viewport.y) * bitmap.width + (X + bitmap.viewport.x)];
            }
        }

        RenderFunc(bitmap.viewport.w, bitmap.viewport.h, std::span<std::uint32_t>(FrameBuffer));
    }

    std::int16_t AudioBuffer[2048] = {};
    const std::int16_t Size = audio_update(AudioBuffer) * 2;

    if (AudioFunc != nullptr)
    {
        AudioFunc(2, std::span<std::int16_t>(AudioBuffer, AudioBuffer + Size));
    }
}

std::vector<std::byte> GenesisPlusGX::SaveState() const
{
    return std::vector<std::byte>();
}

std::error_code GenesisPlusGX::LoadState(std::span<const std::byte> StateData)
{
    return {};
}
