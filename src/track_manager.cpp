//  $Id$
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006 SuperTuxKart-Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <stdio.h>
#include <stdexcept>
#include <algorithm>
#include "file_manager.hpp"
#include "string_utils.hpp"
#include "track_manager.hpp"
#include "track.hpp"
#include "audio/sound_manager.hpp"
#include "translation.hpp"

TrackManager* track_manager = 0;

/** Constructor (currently empty). The real work happens in loadTrack.
 */
TrackManager::TrackManager()
{}   // TrackManager

//-----------------------------------------------------------------------------
/** Delete all tracks.
 */
TrackManager::~TrackManager()
{
    for(Tracks::iterator i = m_tracks.begin(); i != m_tracks.end(); ++i)
        delete *i;
}   // ~TrackManager

//-----------------------------------------------------------------------------
/** Get TrackData by the track identifier.
 *  \param ident Identifier = filename without .track
 */
Track* TrackManager::getTrack(const std::string& ident) const
{
    for(Tracks::const_iterator i = m_tracks.begin(); i != m_tracks.end(); ++i)
    {
        if ((*i)->getIdent() == ident)
            return *i;
    }

    char msg[MAX_ERROR_MESSAGE_LENGTH];
    sprintf(msg, "TrackManager: Couldn't find track: '%s'", ident.c_str() );
    throw std::runtime_error(msg);
}   // getTrack

//-----------------------------------------------------------------------------
/** Marks the specified track as unavailable (i.e. not available on all
 *  clients and server. 
 *  \param ident Track identifier (i.e. track name without .track)
 */
void TrackManager::unavailable(const std::string& ident)
{
    for(Tracks::const_iterator i = m_tracks.begin(); i != m_tracks.end(); ++i)
    {
        if ((*i)->getIdent() == ident)
        {
            m_track_avail[i-m_tracks.begin()] = false;
            return;
        }
    }
    // Do nothing if the track does not exist here ... it's not available.
    return;
}   // unavailable

//-----------------------------------------------------------------------------
/** Sets all tracks that are not in the list a to be unavailable. This is used
 *  by the network manager upon receiving the list of available tracks from
 *  a client.
 *  \param tracks List of all track identifiere (available on a client).
 */
void TrackManager::setUnavailableTracks(const std::vector<std::string> &tracks)
{
    for(Tracks::const_iterator i = m_tracks.begin(); i != m_tracks.end(); ++i)
    {
        if(!m_track_avail[i-m_tracks.begin()]) continue;
        const std::string id=(*i)->getIdent();
        if (std::find(tracks.begin(), tracks.end(), id)==tracks.end())
        {
            m_track_avail[i-m_tracks.begin()] = false;
            fprintf(stderr, "Track '%s' not available on all clients, disabled.\n",
                    id.c_str());
        }   // if id not in tracks
    }   // for all available tracks in track manager

}   // setUnavailableTracks

//-----------------------------------------------------------------------------
/** Returns a list with all track identifiert.
 */
std::vector<std::string> TrackManager::getAllTrackIdentifiers()
{
    std::vector<std::string> all;
    for(Tracks::const_iterator i = m_tracks.begin(); i != m_tracks.end(); ++i)
    {
        all.push_back((*i)->getIdent());
    }
    return all;
}   // getAllTrackNames

//-----------------------------------------------------------------------------
/** Loads all track from the track directory (data/track).
 */
void TrackManager::loadTrackList ()
{
    // Load up a list of tracks - and their names
    std::set<std::string> dirs;
    file_manager->listFiles(dirs, file_manager->getTrackDir(), /*is_full_path*/ true);
    for(std::set<std::string>::iterator dir = dirs.begin(); dir != dirs.end(); dir++)
    {
        if(*dir=="." || *dir=="..") continue;
        std::string config_file;
        try
        {
            // getTrackFile appends dir, so it's opening: *dir/*dir.track
            config_file = file_manager->getTrackFile((*dir)+".track");
        }
        catch (std::exception& e)
        {
            (void)e;   // remove warning about unused variable
            continue;
        }
        FILE *f=fopen(config_file.c_str(),"r");
        if(!f) continue;
        fclose(f);

        Track *track = new Track(config_file);
        m_tracks.push_back(track);
        m_track_avail.push_back(true);
        updateGroups(track);
        // Read music files in that dir as well
        sound_manager->loadMusicFromOneDir(*dir);
    }
}  // loadTrackList
// ----------------------------------------------------------------------------
/** Updates the groups after a track was read in.
 *  \param track Pointer to the new track, whose groups are now analysed.
 */
void TrackManager::updateGroups(const Track* track)
{
    const std::vector<std::string>& new_groups = track->getGroups();
    for(unsigned int i=0; i<new_groups.size(); i++)
    {
        if(m_groups.find(new_groups[i])==m_groups.end())
            m_all_groups.push_back(new_groups[i]);
	m_groups[new_groups[i]].push_back(m_tracks.size()-1);
    }
}   // updateGroups

// ----------------------------------------------------------------------------
