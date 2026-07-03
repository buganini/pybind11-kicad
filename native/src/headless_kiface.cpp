#include <3d_canvas/board_adapter.h>
#include <kiface_base.h>

KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultBackgroundTop = KIGFX::COLOR4D( 0.80, 0.80, 0.90, 1.0 );
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultBackgroundBot = KIGFX::COLOR4D( 0.40, 0.40, 0.50, 1.0 );
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultSilkscreen = KIGFX::COLOR4D( 0.94, 0.94, 0.94, 1.0 );
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultSolderMask = KIGFX::COLOR4D( 0.08, 0.20, 0.14, 0.83 );
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultSolderPaste = KIGFX::COLOR4D( 0.50, 0.50, 0.50, 1.0 );
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultSurfaceFinish = KIGFX::COLOR4D( 0.75, 0.61, 0.23, 1.0 );
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultBoardBody = KIGFX::COLOR4D( 0.43, 0.45, 0.30, 0.90 );
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultComments = KIGFX::COLOR4D( 0.85, 0.85, 0.85, 1.0 );
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultECOs = KIGFX::COLOR4D( 0.70, 0.10, 0.10, 1.0 );

namespace
{
class HEADLESS_KIFACE : public KIFACE_BASE
{
public:
    HEADLESS_KIFACE() :
        KIFACE_BASE( "pybind11_kicad", KIWAY::FACE_PCB )
    {
        m_start_flags = KFCTL_STANDALONE | KFCTL_CLI;
    }

    bool OnKifaceStart( PGM_BASE*, int, KIWAY* ) override
    {
        return true;
    }

    wxWindow* CreateKiWindow( wxWindow*, int, KIWAY*, int ) override
    {
        return nullptr;
    }

    void* IfaceOrAddress( int ) override
    {
        return nullptr;
    }
};
}

KIFACE_BASE& Kiface()
{
    static HEADLESS_KIFACE kiface;
    return kiface;
}
