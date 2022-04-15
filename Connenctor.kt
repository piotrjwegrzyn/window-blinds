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
    var ID: Short,
    var photoResistance: Byte,
    var position: Byte) {
}


fun receiveBroadcastModules(timeoutMilis: Int, port: Int): List<Module> {
    val startTime = System.currentTimeMillis()
    val socket = DatagramSocket(port)

    var modules = mutableListOf<Module>()

    while (true) {
        val packetBuffer = ByteArray(5, {0.toByte()})
        val packet = DatagramPacket(packetBuffer, packetBuffer.size)
        socket.receive(packet)
        packet.getAddress()?.apply {
            val moduleID: Short = (packetBuffer[1].toInt() shl 8 or packetBuffer[2].toInt()).toShort()
            modules.add(Module(packet.getAddress(), moduleID, packetBuffer[3], packetBuffer[4]))
        }

        if (System.currentTimeMillis() > startTime + timeoutMilis) {
            println("Scanning has ended")
            break
        }
    }
    socket.close()
    return modules
}

fun getRequest() {
    // TODO
}

fun setRequest() {
    // TODO
}

fun main() {

    val port = 4210
    val timeout = 1000
    val list = receiveBroadcastModules(timeout, port)

    val output = "IP: %s, ID: %x, Photo resistance: %d, Servo position: %d"
    list.forEach {
        println(output.format(it.ipAddress.toString(), it.ID, it.photoResistance, it.position))
    }
}
