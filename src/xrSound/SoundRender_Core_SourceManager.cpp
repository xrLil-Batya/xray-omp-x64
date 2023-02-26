#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Core.h"
#include "SoundRender_Source.h"
#include <tbb/parallel_for_each.h>

CSoundRender_Source*	CSoundRender_Core::i_create_source		(LPCSTR name)
{
    // Search
    string256 id;
    xr_strcpy(id, name);
    xr_strlwr(id);
    if (strext(id))
        *strext(id) = 0;
    auto it = s_sources.find(id);
    if (it != s_sources.end())
    {
        return it->second;
    }

    // Load a _new one
    CSoundRender_Source* S = new CSoundRender_Source();
    S->load(id);
    s_sources.insert({ id, S });
    return S;
}

void					CSoundRender_Core::i_destroy_source		(CSoundRender_Source*  S)
{
	// No actual destroy at all
}


void CSoundRender_Core::i_create_all_sources()
{

    CTimer T;
    T.Start();

    FS_FileSet flist;
    FS.file_list(flist, "$game_sounds$", FS_ListFiles, "*.ogg");
    const size_t sizeBefore = s_sources.size();

    DECLARE_MT_LOCK(lock);
    const auto processFile = [&](const FS_File& file)
    {
        string256 id;
        xr_strcpy(id, file.name.c_str());

        xr_strlwr(id);
        if (strext(id))
            *strext(id) = 0;

        {
            DECLARE_MT_SCOPE_LOCK(lock);
            const auto it = s_sources.find(id);
            if (it != s_sources.end())
                return;
        }

        CSoundRender_Source* S = new CSoundRender_Source();
        S->load(id);

        DO_MT_LOCK(lock);
        s_sources.insert({ id, S });
        DO_MT_UNLOCK(lock);
    };

    DO_MT_PROCESS_RANGE(flist, processFile);


    Msg("Finished creating %d sound sources. Duration: %d ms",
        s_sources.size() - sizeBefore, T.GetElapsed_ms());

}