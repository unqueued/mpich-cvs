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
import java.text.ChoiceFormat;
import java.awt.*;
import javax.swing.*;
import javax.swing.tree.TreeNode;
import java.util.Map;

import base.drawable.Coord;
import base.drawable.Drawable;
import base.drawable.Primitive;
import base.drawable.Shadow;
import viewer.common.Const;
import viewer.common.Routines;
import viewer.legends.CategoryLabel;


public class InfoPanelForDrawable extends JPanel
{
    private static final Component      STRUT = Box.createHorizontalStrut( 5 );
    private static final Component      GLUE  = Box.createHorizontalGlue();
    private static final String         FORMAT = Const.INFOBOX_TIME_FORMAT;

    private static       DecimalFormat   fmt = null;
    private static       TimeInSecFormat tsfmt = null;

    private              Drawable       drawable;

    public InfoPanelForDrawable( final Map       map_line2treenodes,
                                 final String[]  y_colnames,
                                 final Drawable  dobj )
    {
        super();
        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        drawable = dobj;

        /* Define DecialFormat for the displayed time */
        if ( fmt == null ) {
            fmt = (DecimalFormat) NumberFormat.getInstance();
            fmt.applyPattern( FORMAT );
        }
        if ( tsfmt == null )
            tsfmt = new TimeInSecFormat();

            JPanel       top_panel = new JPanel();
            top_panel.setLayout( new BoxLayout( top_panel, BoxLayout.X_AXIS ) );
            CategoryLabel label_type = new CategoryLabel( dobj.getCategory() );
            top_panel.add( STRUT );
            top_panel.add( label_type );
            top_panel.add( GLUE );
        super.add( top_panel );

            Primitive  prime  = (Primitive) dobj;

            Coord[]        coords = prime.getVertices();
            int            coords_length = coords.length;

            Coord          vertex;
            Integer        lineID;
            YaxisTreeNode  node;
            TreeNode[]     nodes;
            StringBuffer   linebuf;
            StringBuffer   textbuf = new StringBuffer();
            int            num_cols, num_rows;
            double         duration;
            int            idx, ii;

            if ( prime instanceof Shadow ) {
                num_cols = 0;
                num_rows = 3;
                Shadow shade = (Shadow) prime;
                duration = shade.getLatestTime() - shade.getEarliestTime();
                linebuf = new StringBuffer();
                linebuf.append( "duration = (max)" + tsfmt.format(duration) );
                num_cols = linebuf.length();
                textbuf.append( linebuf.toString() );
                linebuf = new StringBuffer();
                duration = coords[coords_length-1].time - coords[0].time;
                linebuf.append( "duration = (ave) " + tsfmt.format(duration) );
                if ( num_cols < linebuf.length() )
                    num_cols = linebuf.length();
                textbuf.append( "\n" + linebuf.toString() );

                    idx     = 0;
                    linebuf = new StringBuffer( "[" + idx + "]: " );
                    vertex  = coords[idx];
                    lineID  = new Integer(vertex.lineID);
                    node    = (YaxisTreeNode) map_line2treenodes.get( lineID );
                    nodes   = node.getPath();
                    linebuf.append( "time = (min)"
                                  + fmt.format(shade.getEarliestTime()) );
                    for ( ii = 1; ii < nodes.length; ii++ )
                        linebuf.append( ", " + y_colnames[ii-1]
                                      + " = " + nodes[ ii ] );
                    if ( num_cols < linebuf.length() )
                        num_cols = linebuf.length();
                    textbuf.append( "\n" + linebuf.toString() );
                    linebuf = new StringBuffer( "[" + idx + "]: " );
                    linebuf.append( "time = (ave) " + fmt.format(vertex.time) );
                    if ( num_cols < linebuf.length() )
                        num_cols = linebuf.length();
                    textbuf.append( "\n" + linebuf.toString() );

                    idx     = coords_length-1;
                    linebuf = new StringBuffer( "[" + idx + "]: " );
                    vertex  = coords[idx];
                    lineID  = new Integer(vertex.lineID);
                    node    = (YaxisTreeNode) map_line2treenodes.get( lineID );
                    nodes   = node.getPath();
                    linebuf.append( "time = (max)"
                                  + fmt.format(shade.getLatestTime()) );
                    for ( ii = 1; ii < nodes.length; ii++ )
                        linebuf.append( ", " + y_colnames[ii-1]
                                      + " = " + nodes[ ii ] );
                    if ( num_cols < linebuf.length() )
                        num_cols = linebuf.length();
                    textbuf.append( "\n" + linebuf.toString() );
                    linebuf = new StringBuffer( "[" + idx + "]: " );
                    linebuf.append( "time = (ave) " + fmt.format(vertex.time) );
                    if ( num_cols < linebuf.length() )
                        num_cols = linebuf.length();
                    textbuf.append( "\n" + linebuf.toString() );

                linebuf = new StringBuffer( "Number of Real Drawables = " );
                linebuf.append( shade.getNumOfRealObjects() );
                if ( num_cols < linebuf.length() )
                    num_cols = linebuf.length();
                textbuf.append( "\n" + linebuf.toString() );
                num_rows++;
            }
            else {
                num_cols = 0;
                num_rows = 3;
                duration = coords[coords_length-1].time - coords[0].time;
                linebuf = new StringBuffer();
                linebuf.append( "duration = " + tsfmt.format(duration) );
                num_cols = linebuf.length();
                textbuf.append( linebuf.toString() );
                for ( idx = 0; idx < coords_length; idx++ ) {
                    linebuf = new StringBuffer( "[" + idx + "]: " );
                    vertex  = coords[idx];
                    lineID  = new Integer(vertex.lineID);
                    node    = (YaxisTreeNode) map_line2treenodes.get( lineID );
                    nodes   = node.getPath();
                    linebuf.append( "time = " + fmt.format(vertex.time) );
                    for ( ii = 1; ii < nodes.length; ii++ )
                        linebuf.append( ", " + y_colnames[ii-1]
                                      + " = " + nodes[ ii ] );
                    if ( num_cols < linebuf.length() )
                        num_cols = linebuf.length();
                    textbuf.append( "\n" + linebuf.toString() );
                }
                
                String info_str = prime.toInfoBoxString().trim();
                if ( info_str.length() > 0 ) {
                    textbuf.append( "\n" + info_str );
                    num_rows++;
                }
            }

            JTextArea text_area = new JTextArea( textbuf.toString() );
            int adj_num_cols    = Routines.getAdjNumOfTextColumns( text_area,
                                                                   num_cols );
            text_area.setColumns( adj_num_cols );
            text_area.setRows( num_rows );
            text_area.setEditable( false );
            text_area.setLineWrap( true );
        super.add( new JScrollPane( text_area ) );
    }

    public Drawable getDrawable()
    {
        return drawable;
    }

    private static class TimeInSecFormat
    {
        private        final double[] LIMITS  = {Double.NEGATIVE_INFINITY, 0.0d,
                                                 1.0E-9, 1.0E-6, 1.0E-3, 1.0d};
        private        final String[] UNITS   = {"-ve", "ps", "ns",
                                                 "us", "ms", "s" };
        private        final String   PATTERN = "#,##0.0##";

        private              DecimalFormat decfmt   = null;
        private              ChoiceFormat  unitfmt  = null;

        public TimeInSecFormat()
        {
            decfmt = (DecimalFormat) NumberFormat.getInstance();
            decfmt.applyPattern( PATTERN );
            unitfmt = new ChoiceFormat( LIMITS, UNITS );
        }

        public String format( double time )
        {
            String unit = unitfmt.format( Math.abs( time ) );
            if ( unit.equals( "s" ) )
                return decfmt.format(time) + " sec";
            else if ( unit.equals( "ms" ) )
                return decfmt.format(time * 1.0E3) + " msec";
            else if ( unit.equals( "us" ) )
                return decfmt.format(time * 1.0E6) + " usec";
            else if ( unit.equals( "ns" ) )
                return decfmt.format(time * 1.0E9) + " nsec";
            else if ( unit.equals( "ps" ) )
                return decfmt.format(time * 1.0E12) + " psec";
            else
                return decfmt.format(time) + " sec";
        }
    }
}
