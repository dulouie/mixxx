#include "engineclipping.h"

/*----------------------------------------------------------------
  A pregaincontrol is ... a pregain.
  ----------------------------------------------------------------*/
EngineClipping::EngineClipping(QRadioButton *BulbClipping)
{
    bulb_clipping = BulbClipping;
    buffer = new CSAMPLE[MAX_BUFFER_LEN];
}

EngineClipping::~EngineClipping()
{
    delete []buffer;
}

CSAMPLE *EngineClipping::process(const CSAMPLE *source, const int buffer_size)
{
    static const int lightswitch = 10;
    static const FLOAT max_amp = 32676;
    static const FLOAT clip = 0.8*max_amp;

    int samples_clipped = 0; 
    for (int i=0; i<buffer_size; i++) {
    	CSAMPLE tmp = source[i];
	    if ((tmp>clip) || (tmp<-clip)) {
            FLOAT sign = 1;
            if (tmp<0) sign = -1;
            buffer[i] = sign*(max_amp - ((max_amp-clip)*(max_amp-clip))/
                        ((max_amp-2*clip)+sign*source[i]));
            samples_clipped++;
        } else
            buffer[i] = source[i];
    }
    if ((samples_clipped > lightswitch) && (!bulb_clipping->isChecked()))
        bulb_clipping->setChecked(true);

    return buffer;
}
