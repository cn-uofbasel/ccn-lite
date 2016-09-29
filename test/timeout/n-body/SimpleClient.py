from KeepaliveRequest import *
#from Tkinter import *
from PIL import Image
import io

class SimpleClient():
    def __init__(self):
        ip = "127.0.0.1"
        port = 9001
        connectionInfo = pyndn.transport.udp_transport.UdpTransport.ConnectionInfo(ip, port)
        transport = pyndn.transport.udp_transport.UdpTransport()
        self.face = pyndn.face.Face(transport, connectionInfo)
        self.pendingRequests = []
        pass

    def run(self):
        self.sendSimulationRequest()
        while self.hasPendingRequests():
            print("Waiting...")
            self.face.processEvents()
            time.sleep(1)
        print("Done.")

    def sendSimulationRequest(self):
        interest = pyndn.Interest(self.simulationName())
        interest.setInterestLifetimeMilliseconds(1000)
        request = KeepaliveRequest(self.face, interest, self.onIntermediateSimulationData, self.onSimulationData,
                                   self.onSimulationTimeout)
        self.sendRequest(request)

    def sendRenderRequest(self):
        interest = pyndn.Interest(self.renderName())
        interest.setInterestLifetimeMilliseconds(5000)
        request = KeepaliveRequest(self.face, interest, None, self.onRenderData, self.onRenderTimeout)
        self.sendRequest(request)

    def onSimulationData(self, request):
        self.sendRenderRequest()

    def onIntermediateSimulationData(self, request):
        pass

    def onSimulationTimeout(self, request):
        pass

    def onRenderData(self, request):
        # file = open('/Users/Bazsi/Desktop/data3.png', 'w')
        buf = request.data.getContent().toBytes()
        # file.write(buf)
        # file.close()
        # renderer = NBodyRenderer()
        # renderer.show()
        # img = Image.frombytes('RGB', (500, 500), buf)
        img = Image.open(io.BytesIO(buf))
        img.show()
        pass

    def onRenderTimeout(self, request):
        pass

    def hasPendingRequests(self):
        for request in self.pendingRequests:
            if request.data is None and request.timeout is False:
                return True
        return False

    def sendRequest(self, request):
        self.pendingRequests.append(request)
        request.send()

    def simulationName(self):
        name = pyndn.Name("/node/nodeD/nfn_service_NBody_SimulationService/")
        name.append("(@x call 7 x '-d' 10 '-s' 100000 '-i' 1)")
        name.append("NFN")
        return name

    def renderName(self):
        simName = "call 7 /node/nodeD/nfn_service_NBody_SimulationService '-d' 10 '-s' 100000 '-i' 1"
        name = pyndn.Name("/node/nodeD/nfn_service_NBody_RenderService/")
        name.append("(@x call 6 x ({}) '-w' 500 '-h' 500)".format(simName))
        name.append("NFN")
        return name


# class NBodyRenderer:
#     def __init__(self):
#         self.master = Tk()
#         self.canvas = Canvas(self.master, width=500, height=500, bg="black")
#
#     def render(self, request):
#         self.canvas.pack()
#         image = ImageTk.PhotoImage(data=request.data.getContent().toBytes())
#         self.canvas.create_image(10, 10, image=image, anchor=NW)



def main(argv):
    client = SimpleClient()
    client.run()


if __name__ == "__main__":
    main(sys.argv)
