/* 
 *  Copyright 2010-2011 Anders Wallin (anders.e.e.wallin "at" gmail.com)
 *  
 *  This file is part of OpenCAMlib.
 *
 *  OpenCAMlib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCAMlib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenCAMlib.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <boost/foreach.hpp>

#include "voronoidiagram.hpp"
#include "numeric.hpp"

namespace ocl
{

int VertexProps::count = 0;

VoronoiDiagram::VoronoiDiagram(double far, unsigned int n_bins) {
    fgrid = new FaceGrid(far, n_bins);
    far_radius=far;
    gen_count=3;
    init();
}

VoronoiDiagram::~VoronoiDiagram() { 
    delete fgrid; 
}

// add one vertex at origo and three vertices at 'infinity' and their associated edges
void VoronoiDiagram::init() {
    double far_multiplier = 6;
    // add init vertices
    HEVertex v0;
    VertexProps v0prop(Point(0,0), UNDECIDED);
    VertexProps v1prop(Point(0, far_multiplier*far_radius), OUT);
    VertexProps v2prop(Point( cos(-5*PI/6)*far_multiplier*far_radius, sin(-5*PI/6)*far_multiplier*far_radius), OUT);
    VertexProps v3prop(Point( cos(-PI/6)*far_multiplier*far_radius, sin(-PI/6)*far_multiplier*far_radius), OUT);
    v0  = hedi::add_vertex( v0prop, g );
    v01 = hedi::add_vertex( v1prop, g );
    v02 = hedi::add_vertex( v2prop, g );
    v03 = hedi::add_vertex( v3prop, g );

    // the locations of the initial generators:
    double gen_mutliplier = 3;
    Point gen2 = Point(cos(PI/6)*gen_mutliplier*far_radius, sin(PI/6)*gen_mutliplier*far_radius);
    Point gen3 = Point(cos(5*PI/6)*gen_mutliplier*far_radius, sin(5*PI/6)*gen_mutliplier*far_radius);
    Point gen1 = Point( 0,-gen_mutliplier*far_radius);
    g[v0].set_J( gen1, gen2, gen3 ); // this sets J2,J3,J4 and pk, so that detH(pl) can be called later
    
    // set clearance-disks for vertices
    g[v0].update_r( gen1 ); g[v0].update_r( gen2 ); g[v0].update_r( gen3 );
    //g[v01].update_r( gen1 ); g[v01].update_r( gen2 ); g[v01].update_r( gen3 );
    //g[v02].update_r( gen1 ); g[v02].update_r( gen2 ); g[v02].update_r( gen3 );
    //g[v03].update_r( gen1 ); g[v03].update_r( gen2 ); g[v03].update_r( gen3 );
    
    // set small clearance disks, so that these are not removed.
    g[v01].update_r( far_radius / 100.0 );
    g[v02].update_r( far_radius / 100.0 );
    g[v03].update_r( far_radius / 100.0 );
    
    std::cout << " v0 " << g[v0].position << "r " << g[v0].r << "\n";
    std::cout << " v01 " << g[v01].position << "r " << g[v01].r << "\n";
    std::cout << " v02 " << g[v02].position << "r " << g[v02].r << "\n";
    std::cout << " v03 " << g[v03].position << "r " << g[v03].r << "\n";
    // add face 1: v0-v1-v2 which encloses gen3
    HEEdge e1 =  hedi::add_edge( v0 , v01 , g);   
    HEEdge e2 =  hedi::add_edge( v01, v02 , g);
    HEEdge e3 =  hedi::add_edge( v02, v0  , g); 
    HEFace f1 =  hedi::add_face( FaceProps(e2, gen3, NONINCIDENT), g ); 
    fgrid->add_face( g[f1] );
    g[e1].face = f1;
    g[e2].face = f1;
    g[e3].face = f1;
    g[e1].next = e2;
    g[e2].next = e3;
    g[e3].next = e1;
    
    // add face 2: v0-v2-v3 which encloses gen1
    HEEdge e4 = hedi::add_edge( v0, v02  , g );   
    HEEdge e5 = hedi::add_edge( v02, v03 , g );
    HEEdge e6 = hedi::add_edge( v03, v0  , g ); 
    HEFace f2 =  hedi::add_face( FaceProps(e5, gen1, NONINCIDENT), g );
    fgrid->add_face( g[f2] );
    g[e4].face = f2;
    g[e5].face = f2;
    g[e6].face = f2;
    g[e4].next = e5;
    g[e5].next = e6;
    g[e6].next = e4;
    
    // add face 3: v0-v3-v1 which encloses gen2
    HEEdge e7 = hedi::add_edge( v0 , v03 , g);   
    HEEdge e8 = hedi::add_edge( v03, v01 , g);
    HEEdge e9 = hedi::add_edge( v01, v0  , g); 
    HEFace f3 =  hedi::add_face( FaceProps(e8, gen2, NONINCIDENT), g );
    fgrid->add_face( g[f3] );
    g[e7].face = f3;
    g[e8].face = f3;
    g[e9].face = f3;
    g[e7].next = e8;
    g[e8].next = e9;
    g[e9].next = e7;
    
    // twin edges
    g[e1].twin = e9;
    g[e9].twin = e1;
    g[e2].twin = HEEdge(); // the outermost edges have invalid twins
    g[e5].twin = HEEdge();
    g[e8].twin = HEEdge();
    g[e3].twin = e4;
    g[e4].twin = e3;
    g[e6].twin = e7;
    g[e7].twin = e6;
    
    assert( vdChecker.isValid(this) );
}



// comments relate to Sugihara-Iri 1994 paper
// this is roughly "algorithm A" from the paper, page 15/50
void VoronoiDiagram::addVertexSite(const Point& p) {
    // only add vertices within the far_radius circle
    assert( p.xyNorm() < far_radius );
    
    // 1) find the closest face and associated generator
    gen_count++;
    HEFace closest_face = fgrid->grid_find_closest_face( p );
    
    // 2) among the vertices on the closest_face
    //    find the seed, which has the lowest detH
    HEVertex v_seed = find_seed_vertex(closest_face, p);
    g[v_seed].status = IN;
    VertexVector v0;
    v0.push_back(v_seed); 
    
    // 3) augment the vertex set to be deleted
    //    - vertex set must remain a tree
    //    - must not delete cycles
    augment_vertex_set_M(v0, p); 
    
    // 4) add new vertices on all edges that connect v0 IN edges to OUT edges
    add_new_voronoi_vertices(v0, p);
    
    // 5) generate new edges that form a loop around the region to be deleted
    HEFace newface = split_faces(p);
    
    // 6) fix the next-pointers in newface, then remove set v0
    remove_vertex_set(v0, newface);
    
    // 7) reset IN/OUT/UNDECIDED for verts, and INCIDENT/NONINCIDENT for faces
    reset_status();

    assert( vdChecker.isValid(this) );
}


// evaluate H on all face vertices and return vertex with the lowest H

HEVertex VoronoiDiagram::find_seed_vertex(HEFace f, const Point& p) {
    VertexVector face_verts = hedi::face_vertices(f,g);
    assert( face_verts.size() >= 3 );
    double minimumH; // safe, because we expect the min H to be negative...
    HEVertex minimalVertex;
    double h;
    bool first = true;
    BOOST_FOREACH( HEVertex q, face_verts) { // go thorugh all the vertices and find the one with smallest detH
        if ( g[q].status != OUT ) {
            h = g[q].detH( p ); // replace with INPredicate(PointGen) INPredicate(LineGen) (?)
            assert( g[q].r_valid );
            double viol = g[q].r - g[q].distance( p );
            std::cout << g[q].index << " viol= " << viol << " detH= " << g[q].detH(p) << "\n";
            
            if ( first || (h<minimumH) ) {
                minimumH = h;
                minimalVertex = q;
                first = false;
            }
        }
    }
    
    if (!(minimumH < 0) ) {
        std::cout << " VD find_seed_vertex() WARNING\n";
        std::cout << " WARNING: searching for seed when inserting " << p << " \n";
        std::cout << " WARNING: closest face is " << f << " with generator " << g[f].generator << " \n";
        std::cout << " WARNING: minimal vd-vertex " << g[minimalVertex].index << " has deth= " << g[minimalVertex].detH( p ) << "\n";
    }
    std::cout << " RETURN: " << g[minimalVertex].index << " viol= " << g[minimalVertex].r-g[minimalVertex].distance(p) << " detH= " << g[minimalVertex].detH(p) << "\n"; 
    //assert( minimumH < 0 );
    return minimalVertex;
}


// evaluate radius on all face vertices and return vertex with the lowest radius
/*
HEVertex VoronoiDiagram::find_seed_vertex(HEFace f, const Point& p) {
    VertexVector face_verts = hedi::face_vertices(f,g);                 
    assert( face_verts.size() >= 3 ); 
    HEVertex minimalVertex;
    double maxViolation;
    double viol;
    bool first = true;
    BOOST_FOREACH( HEVertex v, face_verts) { // go thorugh all the vertices and find the one with smallest detH
        if ( g[v].status != OUT ) { // known OUT-vertices cannot be seeds
            viol = g[v].r - g[v].distance( p );
            std::cout << g[v].index << " viol= " << viol << " detH= " << g[v].detH(p) << "\n";
            if ( first )  {
                maxViolation = viol;
                minimalVertex = v;
                first = false;
            }
            if (viol > maxViolation)  {
                maxViolation = viol;
                minimalVertex = v;
            }
        }
    }
    std::cout << "RETURN: " << g[minimalVertex].index << " viol= " << maxViolation << "\n";
    
    assert( maxViolation > 0);
    return minimalVertex;
}*/


// from the "one million" paper, growing the v0-tree of "IN" vertices by "breadth-first search"
void VoronoiDiagram::augment_vertex_set_M(VertexVector& v0, const Point& p) {
    assert(v0.size()==1);
    std::queue<HEVertex> Q; // FIXME: a priority_queue could/should be used here instead.
    // this woiuld allways examine and decide on the vertex with largest detH, 
    // since that vertex has the most reliable detH sign.
    modified_vertices.push_back( v0[0] );
    markAdjecentFacesIncident( v0[0] );
    pushAdjacentVertices( v0[0] , Q);
    while( !Q.empty() ) {
        HEVertex v = Q.front();      assert( g[v].status == UNDECIDED );
        // mark IN and add to v0 if detH<0 and passes tests. otherwise OUT
        double h = g[v].detH( p ); // replace with INPredicate(PointGen) INPredicate(LineGen)
        if ( h < 0.0 ) { // try to mark IN
        //if ( g[v].inside_r( p ) ) {
            // (C4) v should not be adjacent to two or more IN vertices (this would result in a loop/cycle!)
            // (C5) for an incident face containing v: v is adjacent to an IN vertex on this face
            if ( (adjacentInCount(v) >= 2) || (!incidentFacesHaveAdjacentInVertex(v)) ) {
                //assert( g[v].status == UNDECIDED );
                g[v].status = OUT;
                //std::cout << " v " << hed[v].index << " decision is " << hed[v].type << " IN_COUNT>=2 \n";
            } else {
                g[v].status = IN;
                //std::cout << " v " << hed[v].index << " decision is " << hed[v].type << " (h<0)\n";
                //modified_vertices.push_back( v );
                v0.push_back( v );
                markAdjecentFacesIncident( v );
                pushAdjacentVertices( v , Q);
            }
            //modified_vertices.push_back( v );
        } else { // detH was positive, so mark OUT
            //std::cout << " v " << hed[v].index << " decision is " << hed[v].type << " (h>0)\n";
            //assert( h >= 0.0 );
            //assert( g[v].status == UNDECIDED );
            g[v].status = OUT;
            //modified_vertices.push_back( v );
        }
        modified_vertices.push_back( v );
        Q.pop(); // delete from queue
    }
    
    // sanity-check: for all incident_faces the IN-vertices should be connected
    assert( vdChecker.incidentFaceVerticesConnected( this,  IN ) );
}

// the set v0 are IN vertices that should be removed
// generate new voronoi-vertices on all edges connecting v0 to OUT-vertices
void VoronoiDiagram::add_new_voronoi_vertices(VertexVector& v0, const Point& p) {
    assert( !v0.empty() );
    EdgeVector q_edges = find_in_out_edges(v0); //, OUT); // new vertices generated on these IN-OUT edges
    assert( !q_edges.empty() );
    
    for( unsigned int m=0; m<q_edges.size(); ++m )  {  // create new vertices on all edges q_edges[]
        HEVertex q = hedi::add_vertex(g);
        g[q].status = NEW;
        modified_vertices.push_back(q);
        HEFace face = g[q_edges[m]].face;     assert(  g[face].status == INCIDENT);
        HEEdge twin = g[q_edges[m]].twin;
        HEFace twin_face = g[twin].face;      assert( g[twin_face].status == INCIDENT);
        g[q].set_J( g[face].generator  , g[twin_face].generator  , p); 
        g[q].set_position();
        
        // new minimal-radius approach:
        assert( g[q].r_valid == false );
        g[q].update_r( p );
        g[q].update_r( g[face].generator );
        g[q].update_r( g[twin_face].generator );
        assert( g[q].r_valid == true );

        // check new vertex position, should lie between endpoints of q_edges[m]
        HEVertex trg = hedi::target(q_edges[m], g);
        HEVertex src = hedi::source(q_edges[m], g);
        Point trgP = g[trg].position;
        Point srcP = g[src].position;
        Point newP = g[q].position;
        if (( trgP - srcP ).xyNorm() <= 0 ) {
            /*
            std::cout << "add_new_voronoi_vertices() WARNING ( trgP - srcP ).xyNorm()= " << ( trgP - srcP ).xyNorm() << "\n";
            std::cout << " src = " << srcP << "\n";
            std::cout << " trg= " << trgP << "\n";
            */
            //std::cout << "add_new_voronoi_vertices() WARNING zero-length edge! \n";
            g[q].position = srcP;
        } else {
            assert( ( trgP - srcP ).xyNorm() > 0.0 ); // edge has finite length
            assert( ( trgP - srcP ).dot( trgP - srcP ) > 0.0 ); // length squared
            double t = ((newP - srcP).dot( trgP - srcP )) / ( trgP - srcP ).dot( trgP - srcP ) ;
            bool warn = false;
            //double t_orig=t;
            if (t < 0.0) {
                warn = true;
                t=0.0;
            } else if (t> 1.0) {
                warn = true;
                t=1.0;
            }
            if ( warn ) {
                //std::cout << "add_new_voronoi_vertices() WARNING positioning vertex outside edge! t_orig= " << t_orig << "\n";
                // CORRECT the position....
                g[q].position = srcP + t*( trgP-srcP);
                t = ( g[q].position - srcP).dot( trgP - srcP ) / ( trgP - srcP ).dot( trgP - srcP ) ;
                //std::cout << "add_new_voronoi_vertices() CORRECTED t= " << t << "\n";
            }
            assert( t >= 0.0 );
            assert( t <= 1.0 );
            
            double dtl = g[q].position.xyDistanceToLine(srcP, trgP);
            if (dtl > 1e-3* ( trgP - srcP ).xyNorm() ) {
                //std::cout << "add_new_voronoi_vertices() WARNING new point far from edge!\n";
                //std::cout << "add_new_voronoi_vertices() WARNING edge length= " << ( trgP - srcP ).xyNorm()  << "\n";
                //std::cout << "add_new_voronoi_vertices() WARNING distance to edge= " << dtl  << "\n";
                t = ( g[q].position - srcP).dot( trgP - srcP ) / ( trgP - srcP ).dot( trgP - srcP ) ;
                g[q].position = srcP + t*( trgP-srcP);
                //newP = hed[q].position;
                dtl = g[q].position.xyDistanceToLine(srcP, trgP);
                //std::cout << "add_new_voronoi_vertices() WARNING corrected distance to edge= " << dtl  << "\n";
            }
            assert( dtl < 1e-3* ( trgP - srcP ).xyNorm() );
        }

        // edge q_edges[m]:   IN <-> q NEW <-> OUT
        hedi::insert_vertex_in_edge( q, q_edges[m] , g);
        // sanity check on new vertex
        assert( g[q].position.xyNorm() < 6.1*far_radius); // see init() for placement of the three initial vertices
    }
}

HEFace VoronoiDiagram::split_faces(const Point& p) {
    HEFace newface =  hedi::add_face( FaceProps( HEEdge(), p, NONINCIDENT ), g );
    fgrid->add_face( g[newface] );
    BOOST_FOREACH( HEFace f, incident_faces ) {
        split_face(newface, f); // each INCIDENT face is split into two parts: newface and f
    }
    return newface;
}



// remove vertices in the set v0
void VoronoiDiagram::remove_vertex_set(VertexVector& v0 , HEFace newface) {
    HEEdge current_edge = g[newface].edge; 
    HEEdge start_edge = current_edge;
    // this repairs the next-pointers for newface that are broken.
    bool done = false;
    while (!done) {
        HEVertex current_target = hedi::target( current_edge , g); // an edge on the new face
        HEVertex current_source = hedi::source( current_edge , g);
        BOOST_FOREACH( HEEdge edge, hedi::out_edges( current_target, g ) ) { // loop through potential "next" candidates
            HEVertex out_target = hedi::target( edge , g);
            if ( g[out_target].status == NEW ) { // the next vertex along the face should be "NEW"
                if ( out_target != current_source ) { // but not where we came from
                    g[current_edge].next = edge; // this is the edge we want to take
                    
                    // current and next should belong on the same face
                    
                    if (g[current_edge].face !=  g[ g[current_edge].next ].face) {
                        std::cout << " VD remove_vertex_set() ERROR.\n";
                        std::cout << "current.face = " << g[current_edge].face << " IS NOT next_face = " << g[ g[current_edge].next ].face << std::endl;
                        HEVertex c_trg = hedi::target( current_edge , g);
                        HEVertex c_src = hedi::source( current_edge , g);
                        HEVertex n_trg = hedi::target( g[current_edge].next , g);
                        HEVertex n_src = hedi::source( g[current_edge].next , g);
                        
                        std::cout << "current_edge = " << g[c_src].index << " - " << g[c_trg].index << "\n";
                        std::cout << "next_edge = " << g[n_src].index << " - " << g[n_trg].index << "\n";
                        
                        printFaceVertexTypes( g[current_edge].face );
                        printFaceVertexTypes( g[ g[current_edge].next ].face );
                        
                        std::cout << " printing all incident faces for debug: \n";
                        BOOST_FOREACH( HEFace f, incident_faces ) {
                            printFaceVertexTypes( f );
                        } 
                    }
                    assert( g[current_edge].face ==  g[ g[current_edge].next ].face );
                }
            }
        }

        current_edge = g[current_edge].next; // jump to the next edge
        if ( g[current_edge].next == start_edge )
            done = true;
    }
    // it should now be safe to delete v0
    BOOST_FOREACH( HEVertex v, v0 ) { 
        assert( g[v].status == IN );
        hedi::delete_vertex(v,g); // this also removes edges connecting to v
    }
}

// reset status of modified_vertices and incident_faces
void VoronoiDiagram::reset_status() {
    BOOST_FOREACH( HEVertex v, modified_vertices ) {
        g[v].reset();
    }
    modified_vertices.clear();
    g[v01].status = OUT; // the outer vertices are special.
    g[v02].status = OUT;
    g[v03].status = OUT;
    BOOST_FOREACH(HEFace f, incident_faces ) { 
        g[f].status = NONINCIDENT; 
    }
    incident_faces.clear();
}

// split the face f into one part which is newface, and the other part is the old f
void VoronoiDiagram::split_face(HEFace newface, HEFace f) {
    HEVertex new_source; // this is found as OUT-NEW-IN
    HEVertex new_target; // this is found as IN-NEW-OUT
    // the new vertex on face f connects new_source -> new_target
    HEEdge current_edge = g[f].edge;                             
    assert( f == g[current_edge].face );

    VoronoiVertexStatus currentType = OUT;
    VoronoiVertexStatus nextType  = NEW;
    HEEdge new_previous;
    HEEdge new_next;
    HEEdge twin_next;
    HEEdge twin_previous;
    bool found = false;
    while (!found) {
        HEVertex current_vertex = hedi::target( current_edge , g);
        HEEdge next_edge = g[current_edge].next;
        HEVertex next_vertex = hedi::target( next_edge , g);
        if ( g[current_vertex].status == currentType ) {
            if ( g[next_vertex].status == nextType ) {
                new_source = next_vertex;
                new_previous = next_edge;
                twin_next = g[next_edge].next;
                found = true;
            }
        }
        current_edge = g[current_edge].next;   
    }
    found = false;
    currentType = IN;
    nextType = NEW;
    current_edge = g[f].edge; 
    while (!found) {
        HEVertex current_vertex = hedi::target( current_edge , g);
        HEEdge next_edge = g[current_edge].next;
        HEVertex next_vertex = hedi::target( next_edge , g);
        if ( g[current_vertex].status == currentType ) {
            if ( g[next_vertex].status == nextType ) {
                new_target = next_vertex;
                new_next = g[next_edge].next;
                twin_previous = next_edge;
                found = true;
            }
        }
        current_edge = g[current_edge].next;
    }
    // now connect new_previous -> new_source -> new_target -> new_next
    HEEdge e_new = hedi::add_edge( new_source, new_target , g); // face,next,twin
    g[new_previous].next = e_new;
    g[e_new].next = new_next;
    g[e_new].face = f;
    g[f].edge = e_new; 
    
    // the twin edge that bounds the new face
    HEEdge e_twin = hedi::add_edge( new_target, new_source , g);
    g[twin_previous].next = e_twin;
    g[e_twin].next = twin_next;
    g[e_twin].face = newface;
    g[newface].edge = e_twin; 
    
    g[e_twin].twin = e_new;
    g[e_new].twin = e_twin;
    
}

// given a list inVertices of "IN" vertices, find the adjacent IN-OUT edges 
EdgeVector VoronoiDiagram::find_in_out_edges(VertexVector& inVertices) { 
    assert( !inVertices.empty() );
    EdgeVector output; // new vertices generated on these edges
    BOOST_FOREACH( HEVertex v, inVertices ) {                                   
        assert( g[v].status == IN ); // all verts in v0 are IN
        BOOST_FOREACH( HEEdge edge, hedi::out_edges( v , g) ) {
            HEVertex adj_vertex = hedi::target( edge , g);
            if ( g[adj_vertex].status == OUT ) 
                output.push_back(edge); // this is an IN-OUT edge
        }
    }
    return output;
}

void VoronoiDiagram::pushAdjacentVertices( HEVertex v, std::queue<HEVertex>& Q) {
    BOOST_FOREACH( HEVertex w, hedi::adjacent_vertices(v,g) ) {
        if ( g[w].status == UNDECIDED ) {
            if ( !g[w].in_queue ) { 
                Q.push(w); // push adjacent undecided verts for testing.
                g[w].in_queue=true;
            }
        }
    }
}

int VoronoiDiagram::adjacentInCount(HEVertex v) {
    int in_count=0;
    BOOST_FOREACH( HEVertex w, hedi::adjacent_vertices(v,g) ) {
        if ( g[w].status == IN )
            in_count++;
    }
    return in_count;
}

FaceVector VoronoiDiagram::adjacentIncidentFaces(HEVertex v) {
    FaceVector adj_faces = hedi::adjacent_faces(v,g);
    assert( adj_faces.size() == 3 );
    FaceVector inc_faces;
    BOOST_FOREACH( HEFace f, adj_faces ) {
        if ( g[f].status == INCIDENT )
            inc_faces.push_back( f );
    }
    assert( !inc_faces.empty() );
    return inc_faces;
}

bool VoronoiDiagram::incidentFacesHaveAdjacentInVertex(HEVertex v) {
    bool all_found = true;
    BOOST_FOREACH( HEFace f, adjacentIncidentFaces(v) ) { // check each face f
        // v should be adjacent to an IN vertex on the face
        bool face_found=false;
        BOOST_FOREACH( HEVertex w, hedi::face_vertices(f,g) ) {
            if ( w != v && g[w].status == IN && hedi::has_edge(w,v,g) ) 
                face_found = true;
        }
        if (!face_found)
            all_found=false;
    }
    return all_found;
}


// IN-Vertex v has three adjacent faces, mark nonincident faces incident
// and push them to incident_faces
void VoronoiDiagram::markAdjecentFacesIncident( HEVertex v) {
    assert( g[v].status == IN );
    FaceVector new_adjacent_faces = hedi::adjacent_faces( v, g ); 
    assert( new_adjacent_faces.size()==3 );
    BOOST_FOREACH( HEFace adj_face, new_adjacent_faces ) {
        if ( g[adj_face].status  != INCIDENT ) {
            g[adj_face].status = INCIDENT; 
            incident_faces.push_back(adj_face);
        }
    }
}

void VoronoiDiagram::printFaceVertexTypes(HEFace f) {
    std::cout << " Face " << f << ": ";
    VertexVector face_verts = hedi::face_vertices(f,g);    
    unsigned count=1;
    BOOST_FOREACH( HEVertex v, face_verts ) {
        std::cout << g[v].index  << "(" << g[v].status  << ")";
        if (count != face_verts.size() )
            std::cout << "-";
        count++;
    }
    std::cout << "\n";
}

void VoronoiDiagram::printVertices(VertexVector& q) {
    BOOST_FOREACH( HEVertex v, q) {
        std::cout << g[v].index << " ";
    }
    std::cout << std::endl;
}

std::string VoronoiDiagram::str() const {
    std::ostringstream o;
    o << "VoronoiDiagram (nVerts="<< hedi::num_vertices(g) << " , nEdges="<< hedi::num_edges(g) <<"\n";
    return o.str();
}

} // end namespace
// end file voronoidiagram.cpp
