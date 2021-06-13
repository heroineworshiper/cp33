#include "readertheme.h"



ReaderTheme::ReaderTheme()
 : BC_Theme()
{
}


void ReaderTheme::initialize()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
    extern unsigned char _binary_theme_data_start[];
    set_data(_binary_theme_data_start);
    
    margin = DP(10);
    new_button("load.png",
		"button_up.png",
		"button_hi.png",
		"button_dn.png",
        "load");
    
}







