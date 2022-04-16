/*******************************************
 *  Window blinds
 * April, 2022
 *
 * Author: Piotr J. Węgrzyn
 * Email: piotrwegrzyn@protonmail.com
 *******************************************/

import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress


class Module(
    var ipAddress: InetAddress,
    var id: Short,
    var phr: Byte,
    var ser: Byte) {
    override fun toString(): String {
        return "Module IP: %s, ID: 0x%X, PHR: 0x%02X, SER: 0x%02X".format(this.ipAddress.toString(), this.id, this.phr, this.ser)
    }
}


fun buildID(byte1: Byte, byte2: Byte): Short {
    return ((byte1.toInt() shl 8 and 0xFF00) or (byte2.toInt() and 0xFF)).toShort()
}

fun receiveBroadcastModules(timeoutMilis: Int, socket: DatagramSocket): Map<InetAddress, Module> {
    val startTime = System.currentTimeMillis()
    val modules = mutableMapOf<InetAddress, Module>()

    while (true) {
        val packetBuffer = ByteArray(5) { 0.toByte() }
        val packet = DatagramPacket(packetBuffer, packetBuffer.size)
        socket.receive(packet)
        packet.getAddress()?.apply {
            if (!modules.containsKey(packet.getAddress())) {
                modules[packet.getAddress()] = Module(packet.getAddress(), buildID(packetBuffer[1], packetBuffer[2]), packetBuffer[3], packetBuffer[4])
            }
        }

        if (System.currentTimeMillis() > startTime + timeoutMilis) {
            break
        }
    }
    return modules
}

fun sendPacket(ipAddress: InetAddress, socket: DatagramSocket, port: Int, message: ByteArray) {
    val packet = DatagramPacket(message, message.size, ipAddress, port)
    socket.send(packet)
}

fun getRequest(module: Module, socket: DatagramSocket, port: Int, timeoutMilis: Int): Module {
    val message = ByteArray(5) { 0.toByte() }
    message[0] = 0x02

    sendPacket(module.ipAddress, socket, port, message)
    Thread.sleep(timeoutMilis.toLong())

    val startTime = System.currentTimeMillis()

    while (true) {
        val remoteBuffer = ByteArray(5) { 0.toByte() }
        val packet = DatagramPacket(remoteBuffer, remoteBuffer.size)
        socket.receive(packet)
        packet.getAddress()?.apply {
            if (module.ipAddress == packet.getAddress() && remoteBuffer[0] == 0x10.toByte()) {
                module.id = buildID(remoteBuffer[1], remoteBuffer[2])
                module.phr = remoteBuffer[3]
                module.ser = remoteBuffer[4]
                return module
            }
        }

        if (System.currentTimeMillis() > startTime + timeoutMilis) {
            break
        }
    }
    return module

}

fun setRequest(module: Module, socket: DatagramSocket, port: Int, newID: Short? = null, newSER: Byte? = null) {
    
    val setArray = ByteArray(5, {0.toByte()})
    setArray[0] = 0x01
    
    if (newID != null ){
        setArray[1] = (setArray[1].toInt() or 0x10).toByte()
        setArray[2] = (newID.toInt() shr 8).toByte()
        setArray[3] = (newID.toInt() and 0xFF).toByte()
    }

    if (newSER != null ){
        setArray[1] = (setArray[1].toInt() or 0x1).toByte()
        setArray[4] = newSER
    }

    sendPacket(module.ipAddress, socket, port, setArray)
}

fun main() {

    val port = 4210
    val timeout = 1000
    val newID: Short = 0x1234.toShort()
    val newSER: Byte = 0xAB.toByte()
    val socket = DatagramSocket(port)

    val modulesMap = receiveBroadcastModules(timeout, socket)
    val sampleElement = modulesMap.keys.first()

    modulesMap[sampleElement]?.apply {
        var module = this

        println("Broadcast:")
        println(module)

        module = getRequest(module, socket, port, 5 * timeout)
        println("getRequest before setRequest")
        println(module)
        
        setRequest(module, socket, port, newID=newID, newSER=newSER)

        module = getRequest(module, socket, port, 5 * timeout)
        println("getRequest after setRequest")
        println(module)
    }

    socket.close()
}
