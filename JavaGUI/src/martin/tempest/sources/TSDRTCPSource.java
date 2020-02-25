package martin.tempest.sources;

import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.DataInputStream;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.util.ArrayList;

import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JTextField;

/**
 * This plugin allows for playback of prerecorded files.
 */
 
 public class TSDRTCPSource extends TSDRSource {

	public TSDRTCPSource() {
		super("TCP Source", "TSDRPlugin_TCP", false);
	}

}
