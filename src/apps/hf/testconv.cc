#define WORLD_INSTANTIATE_STATIC_TEMPLATES
#include <world/world.h>

using namespace std;
using namespace madness;

#define N 100

class Array : public WorldObject<Array> {
    vector<double> v;
public:
    /// Make block distributed array with size elements
    Array(World& world, size_t size)
        : WorldObject<Array>(world), v((size-1)/world.size()+1)
    {
        process_pending();
    };

    /// Return the process in which element i resides
    ProcessID owner(size_t i) const {return i/v.size();};

    /// Read element i
    Future<double> read(size_t i) const {
        if (owner(i) == world.rank())
            return Future<double>(v[i-world.rank()*v.size()]);
        else
            return send(owner(i), &Array::read, i);
    };

    /// Write element i
    Void write(size_t i, double value) {
        if (owner(i) == world.rank())
            v[i-world.rank()*v.size()] = value;
        else
            send(owner(i), &Array::write, i, value);
        return None;
    };
};


int main(int argc, char** argv) {
    MPI::Init(argc, argv);
    madness::World world(MPI::COMM_WORLD);
    
    // here is our global object
    Array x(world, N), y(world, N);

    // initialize vectors
    for (int i=0; i < N; i++) 
    {
        x.write(i, i);
        y.write(i, 0.0);
    }
    world.gop.fence();

    // print x   
    if (world.rank() == 0) 
    {
      for (size_t i = 0; i < N; i++)
      {
        printf("%.4f\n", x.read(i).get());
      }
    }

    for (size_t i = 0; i < N; i++)
    {
        Future<double> valxm = (i != 0) ? x.read(i-1) : Future<double>(0.0);
        Future<double> valxp = (i != N) ? x.read(i+1) : Future<double>(0.0);
        Future<double> valx = x.read(i);

        y.write(i, 0.3333333333*valxm.get() + valxp.get() + 0.3333333333*valx.get());
    }
    world.gop.fence();

    if (world.rank() == 0) 
    {
      for (size_t i = 0; i < N; i++)
      {
        printf("%.4f\n", y.read(i).get());
      }
    }
    MPI::Finalize();
}
