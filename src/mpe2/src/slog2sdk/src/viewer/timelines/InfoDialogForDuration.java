/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.text.NumberFormat;
import java.text.DecimalFormat;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

import base.drawable.TimeBoundingBox;
import viewer.common.Const;
import viewer.common.Routines;

public class InfoDialogForDuration extends InfoDialog
{
    private static final Component       STRUT = Box.createHorizontalStrut( 5 );
    private static final Component       GLUE  = Box.createHorizontalGlue();

    private static final String          FORMAT = Const.INFOBOX_TIME_FORMAT;
    private static       DecimalFormat   fmt = null;

    private              TimeBoundingBox timebox;

    public InfoDialogForDuration( final Frame            frame,
                                  final TimeBoundingBox  times )
       
    {
        super( frame, "Duration Info Box", times.getLatestTime() );
        timebox = times;

        /* Define DecialFormat for the displayed time */
        if ( fmt == null ) {
            fmt = (DecimalFormat) NumberFormat.getInstance();
            fmt.applyPattern( FORMAT );
        }
        
        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

            StringBuffer textbuf = new StringBuffer();
            int          num_cols = 0, num_rows = 3;

            StringBuffer linebuf = new StringBuffer();
            linebuf.append( "[0]: time = "
                          + fmt.format(timebox.getEarliestTime()) );
            num_cols = linebuf.length();
            textbuf.append( linebuf.toString() + "\n" );

            linebuf = new StringBuffer();
            linebuf.append( "[1]: time = "
                          + fmt.format(timebox.getLatestTime()) );
            if ( num_cols < linebuf.length() )
                num_cols = linebuf.length();
            textbuf.append( linebuf.toString() + "\n" );
            
            linebuf = new StringBuffer();
            linebuf.append( "duration = "
                          + fmt.format(timebox.getDuration()) );
            if ( num_cols < linebuf.length() )
                num_cols = linebuf.length();
            textbuf.append( linebuf.toString() );

            JTextArea text_area = new JTextArea( textbuf.toString() );
            int adj_num_cols    = Routines.getAdjNumOfTextColumns( text_area,
                                                                   num_cols );
            text_area.setColumns( adj_num_cols );
            text_area.setRows( num_rows );
            text_area.setEditable( false );
            text_area.setLineWrap( true );
        root_panel.add( new JScrollPane( text_area ) );

        root_panel.add( super.getCloseButton() );
    }

    public TimeBoundingBox getTimeBoundingBox()
    {
        return timebox;
    }
}
