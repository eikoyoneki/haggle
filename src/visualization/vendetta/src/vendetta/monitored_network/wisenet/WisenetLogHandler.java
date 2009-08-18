/* Wisenet testbed
 * Uppsala University
 *
 * Wisenet internal release
 *
 * Copyright Wisenet
 */

package vendetta.monitored_network.wisenet;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.vecmath.Point3f;

import vendetta.Vendetta;
import vendetta.visualization.canvases.FloorCanvas;
import vendetta.visualization.other.ConnectionVisualizer;
import vendetta.visualization.other.PacketVisualizer;
import vendetta.visualization.shapes.MonitorNodeShape;

/**
 * A helper class to handle log events.
 * 
 * This class provides methods to parse and maybe visualize log events
 * that were created by a node client. The methods are dependent on
 * the actual log messages and thus depend on the application running
 * on the sensor nodes.
 *
 * @author Frederik Hermans (hermans@inf.fu-berlin.de)
 */
public class WisenetLogHandler {
	/**
	 * The WISENET floor canvas where we do our visualization magic.
	 */
	private FloorCanvas floorCanvas;
	
	/**
	 * Visualizes a sent packet by a animated purple dot.
	 * 
	 * Never use this variable directly, use getPacketVisualizer() instead.
	 */
	private PacketVisualizer packetVisualizer;
	
	/**
	 * Visualizes a connection by a straight line.
	 * 
	 * Never use this variable directly, use getConnectionVisualizer()
	 * instead.
	 */
	private ConnectionVisualizer connectionVisualizer;
	
	/**
	 * The WISENET overlay object.
	 */
	private Wisenet parent;
	
	public WisenetLogHandler(Wisenet parent) {
		this.parent = parent;
	}
	
	/**
	 * Handle a log event generated by a node client.
	 * 
	 * @param msg The log message.
	 * @return true on successful parsing, false otherwise.
	 */
	public boolean parseLogEvent(String msg) {
		String[] split = msg.split("\\s");
		
		if (split.length < 3)
			return false;
		
		/* XXX
		 */
		if ("LE_ALL".equals(split[2])) {
			Matcher m = Pattern.compile(".*(\\d): Rcvd (\\d): (\\d{5})\\.\n?").matcher(msg);
			if (m.matches()) {
				/* Draw a little flying packet ...
				 */

				int destId, srcId;
				SensorNode src, dest;
				Point3f start, end;

				destId = Integer.parseInt(m.group(1));
				srcId = Integer.parseInt(m.group(2));

				src = parent.getNode(srcId);
				dest = parent.getNode(destId);
				
				if (src == null || dest == null) {
					return false;
				}

				start = src.getPosition();
				end = dest.getPosition();
				
				getPacketVisualizer().showPacket(start, end);
			}
			
			m = Pattern.compile(".*(\\d): Rcvd (\\d): Hop (\\d{3})\\..*\n?").matcher(msg);
			if (m.matches()) {
				/* Draw a little flying packet ...
				 */

				int destId, srcId, hops;
				SensorNode src, dest;
				Point3f start, end;

				destId = Integer.parseInt(m.group(1));
				srcId = Integer.parseInt(m.group(2));
				hops = Integer.parseInt(m.group(3));

				src = parent.getNode(srcId);
				dest = parent.getNode(destId);
				
				if (src == null || dest == null) {
					return false;
				}

				start = src.getPosition();
				end = dest.getPosition();
				
				getPacketVisualizer().showPacket(start, end, hops);
			}
			
			m = Pattern.compile(".*(..:..): From (..:..).*\n").matcher(msg);
			if (m.matches()) {
				/* Draw a line to indicate connection.
				 */
				SensorNode pktSrc, pktDest;
				Point3f start, end;
				String logSrcRimeAddress, pktSrcRimeAddress;
				
				logSrcRimeAddress = m.group(1);
				pktSrcRimeAddress = m.group(2);
				
				pktDest = parent.getNodeByRimeAddress(logSrcRimeAddress);
				pktSrc = parent.getNodeByRimeAddress(pktSrcRimeAddress);
				
				if (pktDest == null || pktSrc == null) {
					return false;
				}
				
				start = pktSrc.getPosition();
				end = pktDest.getPosition();
				
				getConnectionVisualizer().packetReceived(start, end);
			}
			
			m = Pattern.compile(".*(..:..) UU\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+).*\n").matcher(msg);
			if (m.matches()) {
				String srcRimeAddress = m.group(1);
				SensorNode node = parent.getNodeByRimeAddress(srcRimeAddress);
				float size;
				
				if (node == null) {
					Wisenet.LOG.error("Packet from unknown node " + srcRimeAddress);
					return true;
				}
				
				int cpu, tx, rx;
				cpu = Integer.parseInt(m.group(2));
				tx  = Integer.parseInt(m.group(4));
				rx  = Integer.parseInt(m.group(5));
				
				size = 1.0f + (float) Math.log10((cpu + tx + rx)) / 4.0f;
				
				MonitorNodeShape s = node.getMonitorNodeShape(0);
				s.setSize(size);
			}
		}

		return true;
	}
	
	/**
	 * Get the floor canvas we're doing our magic on.
	 * 
	 * @return The floor canvas.
	 */
	private FloorCanvas getFloorCanvas() {
		if (floorCanvas != null)
			return floorCanvas;
		
		floorCanvas = (FloorCanvas) Vendetta.getGUI().getCanvas(0);
		return floorCanvas;
	}
	
	/**
	 * Get the packet visualizer we're doing our magic on.
	 * 
	 * @return The packet visualizer.
	 */
	private PacketVisualizer getPacketVisualizer() {
		if (packetVisualizer != null)
			return packetVisualizer;
		
		packetVisualizer = PacketVisualizer.getInstance(getFloorCanvas());
		return packetVisualizer;
	}
	
	/**
	 * Get the connection visualizer we're doing our magic on.
	 * 
	 * @return The connection visualizer.
	 */
	private ConnectionVisualizer getConnectionVisualizer() {
		if (connectionVisualizer != null)
			return connectionVisualizer;
		
		connectionVisualizer = new ConnectionVisualizer(getFloorCanvas());
		return connectionVisualizer;
	}
}
