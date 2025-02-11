#ifndef MESHLAB_DIAMSAMPLER
#define MESHLAB_DIAMSAMPLER
#include <stat_remeshing.h>
#include <vcg/complex/algorithms/clean.h>

class DiamSampler{
    typedef IsoParametrization::CoordType CoordType;
    typedef IsoParametrization::PScalarType PScalarType;

    std::vector<std::vector<std::vector<CoordType> > > SampledPos;
    IsoParametrization *isoParam;
    unsigned int sampleSize;

    void DeAllocatePos()
    {
        ///positions
        for (unsigned int i=0;i<SampledPos.size();i++)
        {
            for (unsigned int j=0;j<SampledPos[i].size();j++)
                SampledPos[i][j].clear();
            SampledPos[i].clear();
        }
        SampledPos.clear();
    }

    void AllocatePos(const int & sizeSampl)
    {
        ///positions
        AbstractMesh *domain=isoParam->AbsMesh();
        int num_edges=0;
        for (unsigned int i=0;i<domain->face.size();i++)
        {
            AbstractFace *f=&domain->face[i];
            for (int j=0;j<3;j++)
                if (f->FFp(j)>f)
                    num_edges++;
        }

        SampledPos.resize(num_edges);
        for (unsigned int i=0;i<SampledPos.size();i++)
        {
            SampledPos[i].resize(sizeSampl);
            for (unsigned int j=0;j<SampledPos[i].size();j++)
                SampledPos[i][j].resize(sizeSampl);
        }
    }

    CoordType AveragePos(const std::vector<ParamFace*> &faces,
        std::vector<CoordType> &barys)
    {
        CoordType pos=CoordType(0,0,0);
        for (unsigned int i=0;i<faces.size();i++)
        {
            pos+=(faces[i]->P(0)*barys[i].X()+
                faces[i]->P(1)*barys[i].Y()+
                faces[i]->P(2)*barys[i].Z());
        }
        pos/=(PScalarType)faces.size();
        return pos;
    }

    int n_diamonds;
    int inFace;
    int inEdge;
    int inStar;
    int n_merged;
public:

    ///initialize the sampler
    void Init(IsoParametrization *_isoParam)
    {
        isoParam=_isoParam;
    }

    template <class OutputMesh>
    void GetMesh(OutputMesh &SaveMesh)
    {

        typedef typename OutputMesh::VertexType MyVertex;

        SaveMesh.Clear();

        SaveMesh.vert.reserve(SampledPos.size()*
            sampleSize*
            sampleSize);

        SaveMesh.face.reserve(SampledPos.size()*
            (sampleSize-1)*
            (sampleSize-1)*2);

        SaveMesh.vn=0;
        SaveMesh.fn=0;

        ///supposed to be the same everywhere
        ///allocate space
        std::vector<std::vector<MyVertex*> > vertMatrix;
        vertMatrix.resize(sampleSize);
        for (unsigned int i=0;i<sampleSize;i++)
            vertMatrix[i].resize(sampleSize);

        ///add vertices
        for (unsigned int i=0;i<SampledPos.size();i++)
        //for (unsigned int i=11;i<12;i++)
        {
            ///add vertices & positions
            for (unsigned int j=0;j<sampleSize;j++)
                for (unsigned int k=0;k<sampleSize;k++)
                {
                  vcg::tri::Allocator<OutputMesh>::AddVertex(SaveMesh,SampledPos[i][j][k]);
                  vertMatrix[j][k]=&SaveMesh.vert.back();
                }
                /*printf("samppling %d\n",i);*/
                ///add faces
                for (unsigned int j=0;j<sampleSize-1;j++)
                    for (unsigned int k=0;k<sampleSize-1;k++)
                    {
                        MyVertex *v0=vertMatrix[j][k];
                        MyVertex *v1=vertMatrix[j+1][k];
                        MyVertex *v2=vertMatrix[j+1][k+1];
                        MyVertex *v3=vertMatrix[j][k+1];
                        assert(vcg::tri::IsValidPointer(SaveMesh,v0));
                        assert(vcg::tri::IsValidPointer(SaveMesh,v1));
                        assert(vcg::tri::IsValidPointer(SaveMesh,v2));
                        assert(vcg::tri::IsValidPointer(SaveMesh,v3));
                        vcg::tri::Allocator<OutputMesh>::AddFace(SaveMesh,v0,v1,v3);
                        vcg::tri::Allocator<OutputMesh>::AddFace(SaveMesh,v1,v2,v3);
                    }

        }
        ///get minimum edge size
        typename OutputMesh::ScalarType minE,maxE;
        MaxMinEdge<OutputMesh>(SaveMesh,minE,maxE);
        /*int num_tri=SampledPos.size()*sampleSize*sampleSize*2;
        PScalarType Area_mesh=Area<OutputMesh>(SaveMesh);
        PScalarType Edge=sqrt((((Area_mesh/(PScalarType)num_tri)*4.0)/(PScalarType)sqrt(3.0)));*/
        n_merged=vcg::tri::Clean<OutputMesh>::MergeCloseVertex(SaveMesh,(PScalarType)minE*(PScalarType)0.9);
        vcg::tri::UpdateNormal<OutputMesh>::PerVertexNormalized(SaveMesh);
        /*Log("Merged %d vertices\n",merged);*/
    }

    //typedef enum SampleAttr{SMNormal, SMColor, SMPosition};

    ///sample the parametrization
    bool SamplePos(const int &size)
    {
        if (size<2)
            return false;
        sampleSize=size;
        DeAllocatePos();//clear old data
        AllocatePos(size);	///allocate for new one
        inFace=0;
        inEdge=0;
        inStar=0;
        int global=0;

        ///start sampling values
        /*Log("Num Diamonds: %d \n",SampledPos.size());*/


        for (unsigned int diam=0;diam<SampledPos.size();diam++)
            for (unsigned int j=0;j<sampleSize;j++)
                for (unsigned int k=0;k<sampleSize;k++)
                {
                    vcg::Point2<PScalarType> UVQuad,UV;
                    UVQuad.X()=(PScalarType)j/(PScalarType)(sampleSize-1);
                    UVQuad.Y()=(PScalarType)k/(PScalarType)(sampleSize-1);
                    int I;
                    //printf("Quad: %d,%f,%f \n",diam,UV.X(),UV.Y());
                    ///get coordinate in parametric space
                    isoParam->inv_GE1Quad(diam,UVQuad,I,UV);
                    //printf("Alfabeta: %d,%f,%f \n",I,UV.X(),UV.Y());
                    ///and sample
                    std::vector<ParamFace*> faces;
                    std::vector<CoordType> barys;
                    int domain=isoParam->Theta(I,UV,faces,barys);

                    if (domain==0)
                        inFace++;
                    else
                        if (domain==1)
                            inEdge++;
                        else
                            if (domain==2)
                                inStar++;

                    global++;
                    //printf("Find in domain: %d \n",domain);
                    ///store value
                    CoordType val=AveragePos(faces,barys);
                    /*if (domain==2)
                    val=CoordType(0,0,0);*/
                    SampledPos[diam][j][k]=val;
                }
                return true;
                /*#ifndef _MESHLAB
                printf("In Face: %f \n",(PScalarType)inFace/(PScalarType)global);
                printf("In Diamond: %f \n",(PScalarType)inEdge/(PScalarType)global);
                printf("In Star: %f \n",(PScalarType)inStar/(PScalarType)global);
                #endif*/
                /*Log("In Face: %f \n",(PScalarType)inFace/(PScalarType)global);
                Log("In Diamond: %f \n",(PScalarType)inEdge/(PScalarType)global);
                Log("In Star: %f \n",(PScalarType)inStar/(PScalarType)global);*/
    }

    void getResData(int &_n_diamonds,int &_inFace,
                    int &_inEdge,int &_inStar,int &_n_merged)
    {
        _n_diamonds=n_diamonds;
        _inFace=inFace;
        _inEdge=inEdge;
        _inStar=inStar;
        _n_merged=n_merged;
    }

    void GetCoords(std::vector<CoordType> &positions)
    {
        for (unsigned int diam=0;diam<SampledPos.size();diam++)
            for (unsigned int j=0;j<sampleSize;j++)
                for (unsigned int k=0;k<sampleSize;k++)
                    positions.push_back(SampledPos[diam][j][k]);
    }


};
#endif // MESHLAB_DIAMSAMPLER
