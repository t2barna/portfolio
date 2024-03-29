import numpy as np                              # Alapvető matematikai eszköztár
import matplotlib.pyplot as plt                 # Plotoláshoz
import matplotlib                   
from matplotlib import animation                # Animációk készítéséhez
from matplotlib.animation import PillowWriter   # .gif formátumba való exportáláshoz
from itertools import combinations

target_loc = r"C://saját/docs/python/kin_gaz.gif"
T = 3
dt = 0.005

N = 250
radius = 0.005
#size_space = 1
max_vel = 1.5

np.random.seed(1324354657)
dpi = 300
l = 5

class vec2:
    def __init__(self, x = 0, y = 0):
        self.x = x
        self.y = y
    def __init__(self, arr):
        self.x = arr[0]
        self.y = arr[1]
    def abs(self):
        return np.sqrt(self.x**2 + self.y**2)
    
    def __mul__(self, other):
        if type(other) == type(self):
            return self.x * other.x + self.y * other.y
        else:
            return vec2([self.x * other, self.y * other])
    
    def __sub__(self, other):
        return vec2([self.x - other.x, self.y - other.y])
    
    def __truediv__(self, other):
        return vec2([self.x / other, self.y / other])
    
    def __add__(self, other):
        return vec2([self.x + other.x, self.y + other.y])
    
    def __repr__(self):
        return f"{self.x}, {self.y}"
    
    def v2t(self):
        return (self.x, self.y)


class particle():
    def __init__(self, size = 0.1):
        #self.x_pos = np.random()
        #self.y_pos = np.random()
        #self.x_vel = np.random.uniform(-max_vel, max_vel)
        #self.y_vel = np.random.uniform(-max_vel, max_vel)
        self.rad = size

        #self.pos = np.random.random(size = 2)
        #self.vel = np.random.uniform(-max_vel, max_vel, size = 2)
        self.pos = vec2(np.random.random(size = 2))
        self.vel = vec2(np.random.uniform(-max_vel, max_vel, size = 2))
    def get_pos(self):
        return self.pos
    def set_pos(self, x, y):
        self.pos.x = x
        self.pos.y = y

    def get_vel(self):
        return (self.vel)
    def set_x_vel(self, vx):
        self.vel.x = vx,
    def set_y_vel(self, vy):
        self.vel.y = vy,
    def set_vel(self, vel):
        self.vel = vel

    def move(self, dt):
        self.pos.x += self.vel.x * dt
        self.pos.y += self.vel.y * dt
        self.bounce_boundary()
    
    def get_rad(self):
        return self.rad
    
    def bounce_boundary(self):
        if self.pos.x <= self.rad:
            self.pos.x = self.rad
            self.vel.x = -self.vel.x
        if self.pos.y <= self.rad:
            self.pos.y = self.rad
            self.vel.y = -self.vel.y
        if self.pos.x >= 1 - self.rad:
            self.pos.x = 1 - self.rad
            self.vel.x = -self.vel.x
        if self.pos.y >= 1 - self.rad:
            self.pos.y = 1 - self.rad
            self.vel.y = -self.vel.y
    
class space:
    def __init__(self, numPoints = 20):
        self.numP = numPoints
        self.particles = []
        for i in range(numPoints):
            self.particles.append(particle(radius))

        self.create_pairing()

        self.fig, self.ax = plt.subplots()
        self.fig.set_size_inches(l, l)
        self.ax.set_xlim(0, 1)
        self.ax.set_ylim(0, 1)
        plt.xticks([])
        plt.yticks([])
        self.animation = animation.FuncAnimation(
                        self.fig,
						func = self.update,
						frames = np.arange(0, T, dt),
						interval = 10)
        self.animation.save(target_loc, writer = 'pillow', fps=30, dpi = dpi)


    def create_pairing(self):
        self.pairings = []
        for i in range(self.numP):
            for j in range(i + 1, self.numP):
                self.pairings.append((self.particles[i], self.particles[j]))
                #print(f"{i}, {j}")
        return self.pairings

    def draw(self, c):
        self.ax.clear()
        self.ax.set_xlim(0, 1)
        self.ax.set_ylim(0, 1)
#        M = self.ax.transData.get_matrix()
#        scale = M[0,0]
#        for i in range(N):
#            x = (self.particles[i].get_pos()).x
#            y = (self.particles[i].get_pos()).y
            #l inch, l * dpi px, radius * l * dpi px 
#            size = scale * np.pi * self.particles[i].get_rad()**2
#            self.ax.scatter(x, y, s = size, c=c)

        circles = [plt.Circle(part.get_pos().v2t(), radius=part.get_rad(), linewidth=0) for part in self.particles]
        c = matplotlib.collections.PatchCollection(circles)
        self.ax.add_collection(c)


    def update(self, i):
        for p in self.pairings:
            r1 = p[0].get_pos()#self.particles[p[0]].get_pos()
            r2 = p[1].get_pos()#self.particles[p[2]].get_pos()
            v1 = p[0].get_vel()#self.particles[p[0]].get_pos()
            v2 = p[1].get_vel()
            rad1 = p[0].get_rad()#self.particles[p[0]].get_rad()
            rad2 = p[1].get_rad()#self.particles[p[2]].get_rad()
            #dist = np.sqrt((r[i, 0] - r[j, 0])**2 + (r[i, 1] - r[j, 1])**2)
            dist2 = (r1.x - r2.x)**2 + (r1.y - r2.y)**2
            if dist2 <= (rad1 + rad2)**2 and ((v1 * (r2 - r1)) > 0 and (v2 * (r1 - r2)) > 0):       #if the particles are closer than the sum of the radii and the velocities do not point apart
                n_v1 = v1 - (r1 - r2) * ((v1 - v2) * ( r1 - r2)) / (r1 - r2).abs()**2
                n_v2 = v2 - (r2 - r1) * ((v2 - v1) * ( r2 - r1)) / (r2 - r1).abs()**2
                p[0].set_vel(n_v1)
                p[1].set_vel(n_v2)
                #print(n_v2)

            #p[0].move(dt)
            #p[1].move(dt)
        for i in range(N):
            self.particles[i].move(dt)
        self.draw("red")

            






space = space(N)