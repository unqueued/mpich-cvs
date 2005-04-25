/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.histogram;

import java.text.NumberFormat;
import java.text.DecimalFormat;
import java.awt.*;
import javax.swing.*;
import javax.swing.border.*;

import base.drawable.Topology;
import base.drawable.CategoryWeight;
import base.statistics.CategoryTimeBox;
import base.statistics.TimeAveBox;
import base.statistics.Summarizable;
import base.topology.SummaryState;
import viewer.common.Const;
import viewer.common.Routines;
import viewer.common.Parameters;
import viewer.legends.CategoryLabel;
import viewer.zoomable.TimeFormat;


public class InfoPanelForSummary extends JPanel
{
    private static final Component      STRUT = Box.createHorizontalStrut( 10 );
    private static final Component      GLUE  = Box.createHorizontalGlue();

    private static final String         FORMAT = "0.0#";
    private static       DecimalFormat  nfmt   = null;
    private static       TimeFormat     tfmt   = null;

    private static       Border         Normal_Border  = null;



    public InfoPanelForSummary( final JTree         tree_view,
                                final String[]      y_colnames,
                                final Summarizable  summarizable )
    {
        super();
        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        /* Define DecialFormat for the TimeAve Number of Real Objects */
        if ( nfmt == null ) {
            nfmt = (DecimalFormat) NumberFormat.getInstance();
            nfmt.applyPattern( FORMAT );
        }
        if ( tfmt == null )
            tfmt = new TimeFormat();
        if ( Normal_Border == null ) {
            /*
            Normal_Border = BorderFactory.createCompoundBorder(
                            BorderFactory.createRaisedBevelBorder(),
                            BorderFactory.createLoweredBevelBorder() );
            */
            Normal_Border = BorderFactory.createEtchedBorder();
        }

        Topology  topo          = summarizable.getTopology();
        int       rowID_start   = summarizable.getStartRowID();
        int       rowID_final   = summarizable.getFinalRowID();
        Object    clicked_obj   = summarizable.getClickedObject();
        String    type_name     = " Summary " + topo + " ";

        Border        border     = null;
        CategoryLabel label_type = null;
        if ( clicked_obj instanceof CategoryTimeBox ) {
            CategoryWeight  twgt;
            twgt       = ( (CategoryTimeBox) clicked_obj ).getCategoryWeight();
            label_type = new CategoryLabel( twgt.getCategory() ); 
            border     = BorderFactory.createTitledBorder(
                                       Normal_Border, type_name,
                                       TitledBorder.LEFT, TitledBorder.TOP,
                                       Const.FONT, Color.magenta );
        }
        else {  // if ( clicked_obj instanceof TimeAveBox )
            label_type = new CategoryLabel( type_name, topo,
                                            SummaryState.ForeColor );
            border     = Normal_Border;
        }

        // Set the CategoryLabel Icon
        Dimension     panel_max_size;
        JPanel        top_panel  = new JPanel();
        top_panel.setLayout( new BoxLayout( top_panel, BoxLayout.X_AXIS ) );
            top_panel.add( STRUT );
            top_panel.add( label_type );
            top_panel.add( GLUE );
            top_panel.setBorder( border );
        top_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        panel_max_size        = top_panel.getPreferredSize();
        panel_max_size.width  = Short.MAX_VALUE;
        top_panel.setMaximumSize( panel_max_size );
        super.add( top_panel );

        // Determine the text of the drawable
        TextAreaBuffer  textbuf;
        int             num_cols, num_rows;
        textbuf = new TextAreaBuffer( tree_view, y_colnames );
        textbuf.setEndRowIDsText( topo, rowID_start, rowID_final );
        if ( clicked_obj instanceof CategoryTimeBox )
            textbuf.setCategoryTimeBoxText( (CategoryTimeBox) clicked_obj );
        else // if ( clicked_obj instanceof TimeAveBox )
            textbuf.setTimeAveBoxText( (TimeAveBox) clicked_obj );
        textbuf.finalized();
        num_cols  = textbuf.getColumnCount();
        num_rows  = textbuf.getRowCount();

        // Set the TextArea
        JTextArea  text_area;
        int        adj_num_cols;
        text_area    = new JTextArea( textbuf.toString() );
        adj_num_cols = Routines.getAdjNumOfTextColumns( text_area, num_cols );
        num_cols     = (int) Math.ceil( adj_num_cols * 85.0d / 100.0d );
        text_area.setColumns( num_cols );
        text_area.setRows( num_rows );
        text_area.setEditable( false );
        text_area.setLineWrap( true );
        JScrollPane scroller = new JScrollPane( text_area );
        scroller.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.add( scroller );
    }



    private class TextAreaBuffer
    {
        private              JTree          tree_view;
        private              String[]       y_colnames;
        private              StringBuffer   strbuf;
        private              String         strbuf2str;
        private              int            num_cols;
        private              int            num_rows;

        public TextAreaBuffer( final JTree     in_tree_view,
                               final String[]  in_y_colnames )
        {
            tree_view           = in_tree_view;
            y_colnames          = in_y_colnames;
            strbuf              = new StringBuffer();
            strbuf2str          = null;

            // Initialize num_cols and num_rows.
            num_cols            = 0;
            num_rows            = 0;
        }

        // this.finalized() needs to be called before
        // getColumnCount()/getRowCount()/toString()
        public void finalized()
        {
            int num_lines;
            strbuf2str = strbuf.toString();
            num_lines  = this.getNumOfLines();
            if ( num_lines <= 3 )
                num_rows = 3;
            else
                num_rows = 4;
        }

        public int getColumnCount()
        { return num_cols; }

        public int getRowCount()
        { return num_rows; }

        public String toString()
        { return strbuf2str; }

        private int getNumOfLines()
        {
            int num_lines;
            int str_length;
            int ipos;
            if ( strbuf2str != null ) {
                num_lines  = 1;
                ipos       = 0;
                str_length = strbuf2str.length();
                while ( ipos >= 0 && ipos < str_length ) {
                    ipos = strbuf2str.indexOf( '\n', ipos );
                    if ( ipos >= 0 ) {
                        num_lines++;
                        ipos++;
                    }
                }
                return num_lines;
            }
            else
                return -1;
        }

        public void  setEndRowIDsText( final Topology  topo,
                                             int       rowID_start,
                                             int       rowID_final )
        {
            StringBuffer      linebuf;
            Object[]          y_labels;
            int               idx;

            if ( topo.isArrow() ) {
                linebuf  = new StringBuffer( "Between" );
                y_labels = tree_view.getPathForRow( rowID_start ).getPath();
                for ( idx = 1; idx < y_labels.length; idx++ )
                     linebuf.append( " " + y_colnames[ idx-1 ]
                                   + "=" + y_labels[ idx ] );
                if ( num_cols < linebuf.length() )
                    num_cols = linebuf.length();
                num_rows++;
                strbuf.append( linebuf.toString() );

                linebuf  = new StringBuffer( "   and   " );
                y_labels = tree_view.getPathForRow( rowID_final ).getPath();
                for ( idx = 1; idx < y_labels.length; idx++ )
                     linebuf.append( " " + y_colnames[ idx-1 ]
                                   + "=" + y_labels[ idx ] );
                if ( num_cols < linebuf.length() )
                    num_cols = linebuf.length();
                num_rows++;
                strbuf.append( "\n" + linebuf.toString() );
            }
            else {
                linebuf  = new StringBuffer( "At" );
                y_labels = tree_view.getPathForRow( rowID_start ).getPath();
                for ( idx = 1; idx < y_labels.length; idx++ )
                     linebuf.append( " " + y_colnames[ idx-1 ]
                                   + "=" + y_labels[ idx ] );
                if ( num_cols < linebuf.length() )
                    num_cols = linebuf.length();
                num_rows++;
                strbuf.append( linebuf.toString() );
            }
        }

        private int   getPrintStatus( final Topology topo )
        {
            if ( topo.isState() )
                if ( SummaryState.isDisplayTypeEqualWeighted() )
                    return CategoryWeight.PRINT_ALL_RATIOS;
                else
                    if ( SummaryState.isDisplayTypeExclusiveRatio() )
                        return CategoryWeight.PRINT_EXCL_RATIO;
                    else
                        return CategoryWeight.PRINT_INCL_RATIO;
            else  //  Non state, i.e. arrow and event;
                return CategoryWeight.PRINT_INCL_RATIO;
        }

        public void  setCategoryTimeBoxText( final CategoryTimeBox typebox )
        {
            CategoryWeight  twgt;
            String          twgt_str;
            int             print_status;
 
            twgt          = typebox.getCategoryWeight();
            print_status  = getPrintStatus( twgt.getCategory().getTopology() );
            num_rows++;
            strbuf.append( "\n" + CategoryWeight.getPrintTitle(print_status) );

            twgt_str      = twgt.toInfoBoxString( print_status );
            if ( num_cols < twgt_str.length() )
                num_cols = twgt_str.length();
            num_rows++;
            strbuf.append( "\n" + twgt_str );
        }

        // For Shadow Primitive
        public void setTimeAveBoxText( final TimeAveBox  avebox )
        {
            StringBuffer       linebuf;
            Topology           avebox_topo;
            CategoryTimeBox[]  typeboxes;
            CategoryWeight     twgt;
            String             twgt_str;
            int                print_status;
            int                idx;

            typeboxes     = avebox.arrayOfCategoryTimeBoxes(); 
            twgt          = typeboxes[ 0 ].getCategoryWeight();
            avebox_topo   = twgt.getCategory().getTopology(); 

         // linebuf = new StringBuffer("Averaged Number of Real Drawables = ");
            linebuf = new StringBuffer( "Averaged Number of Real " );
            linebuf.append( avebox_topo + "s = " );
            linebuf.append( nfmt.format( avebox.getAveNumOfRealObjects() ) );
            if ( num_cols < linebuf.length() )
                num_cols = linebuf.length();
            num_rows++;
            strbuf.append( "\n" + linebuf.toString() );
    
            print_status  = getPrintStatus( avebox_topo );
            num_rows++;
            strbuf.append( "\n" + CategoryWeight.getPrintTitle(print_status) );
            for ( idx = 0; idx < typeboxes.length; idx++ ) {
                twgt     = typeboxes[ idx ].getCategoryWeight();
                twgt_str = twgt.toInfoBoxString( print_status );
                if ( num_cols < twgt_str.length() )
                    num_cols = twgt_str.length();
                num_rows++;
                strbuf.append( "\n" + twgt_str );
            }                               
        }
    }   //  End of   private class TextAreaBuffer
}
